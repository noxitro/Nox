/// @file nox_build_insights.cpp
/// @brief C++ Build Insights SDK でビルドを計測し、重いヘッダ・テンプレート・関数を JSON に集計する。
///
/// 使い方 (管理者権限が要る):
///   nox_build_insights start <session> [--no-templates]
///   (ビルド)
///   nox_build_insights stop <session> <raw.etl>
///   nox_build_insights analyze <raw.etl> <out.json>
///
/// VS 同梱の計測ツールを使わず SDK を直接呼んでいるのは、次の 2 点のため。
///   - ランナーの VS の版に左右されない (SDK の版はここで固定できる)
///   - 並走するビルドで按分した時間 (wall clock time responsibility) を集計に使える
///
/// 出力した JSON は .github/scripts/build-insights-report.py が HTML にする。
/// パスの正規化 (リポジトリ相対など) は Python 側で行う。
///
/// 集計の方針:
///   - ヘッダ: 翻訳単位 (front-end pass) ごとにインクルードツリーを組み、パスの終わりで集計する。
///     PCH を作るパスかどうかはパスの終わりまでに分かるので、その時点で「PCH 内」を数えられる。
///   - テンプレート: シンボルのキーはトレース内で一意だが、同じ型でもパスごとに別のキーになる。
///     名前 (SymbolName) はそのパスのインスタンス化が終わった後に届くので、パスの終わりで
///     名前を引いてから、パスをまたいだ集計は名前で行う。
///     同じ primary template の祖先を持つインスタンス化は inclusive に足さない (再帰の二重計上を避ける)。
///   - 関数: 名前 (装飾名) で集計する。/GL の構成ではコード生成がリンク時 (LTCG) に移る。

#include <Windows.h>
#include <DbgHelp.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <CppBuildInsights.hpp>

namespace bi = Microsoft::Cpp::BuildInsights;
namespace act = Microsoft::Cpp::BuildInsights::Activities;
namespace se = Microsoft::Cpp::BuildInsights::SimpleEvents;

namespace
{
	/// 出力するテンプレート・関数の上限。HTML に埋め込むデータ量を抑える。
	constexpr size_t kMaxTemplates = 1500;
	constexpr size_t kMaxSpecializations = 25;
	constexpr size_t kMaxTemplateFiles = 10;
	constexpr size_t kMaxFunctions = 1500;
	constexpr size_t kMaxParents = 10;
	constexpr size_t kMaxInlinees = 5;
	/// インクルードツリーでこれより短いノードは親の「その他」に畳む。
	constexpr long long kTreeThresholdUs = 500;

	long long ToUs(std::chrono::nanoseconds ns)
	{
		return static_cast<long long>(ns.count() / 1000);
	}

	std::string WideToUtf8(const wchar_t* s)
	{
		if (s == nullptr || *s == L'\0')
		{
			return {};
		}
		const int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 1)
		{
			return {};
		}
		std::string out(static_cast<size_t>(len - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, s, -1, out.data(), len, nullptr, nullptr);
		return out;
	}

	/// 過長表現・サロゲート・U+10FFFF 超も不正として扱う (MB_ERR_INVALID_CHARS)。
	/// 先頭ビットだけ見る判定だと、後段の Python が読めないバイト列を JSON に通してしまう。
	bool IsValidUtf8(const char* s)
	{
		return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, nullptr, 0) > 0;
	}

	/// イベントの char* はシステムのコードページのことがある。UTF-8 として正しければそのまま使う。
	std::string NarrowToUtf8(const char* s)
	{
		if (s == nullptr)
		{
			return {};
		}
		if (IsValidUtf8(s))
		{
			return s;
		}
		const int wlen = MultiByteToWideChar(CP_ACP, 0, s, -1, nullptr, 0);
		if (wlen <= 1)
		{
			return {};
		}
		std::wstring w(static_cast<size_t>(wlen - 1), L'\0');
		MultiByteToWideChar(CP_ACP, 0, s, -1, w.data(), wlen);
		return WideToUtf8(w.c_str());
	}

	std::string ToLowerAscii(std::string s)
	{
		for (char& c : s)
		{
			if (c >= 'A' && c <= 'Z')
			{
				c = static_cast<char>(c - 'A' + 'a');
			}
		}
		return s;
	}

	std::string Undecorate(const std::string& decorated)
	{
		if (decorated.empty() || decorated[0] != '?')
		{
			return decorated;
		}
		char buffer[8192];
		const DWORD flags = UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_MEMBER_TYPE |
			UNDNAME_NO_THROW_SIGNATURES | UNDNAME_NO_FUNCTION_RETURNS | UNDNAME_NO_ALLOCATION_MODEL |
			UNDNAME_NO_ALLOCATION_LANGUAGE;
		const DWORD len = UnDecorateSymbolName(decorated.c_str(), buffer, static_cast<DWORD>(sizeof(buffer)), flags);
		if (len == 0)
		{
			return decorated;
		}
		return std::string(buffer, len);
	}

	const char* ResultCodeName(bi::RESULT_CODE rc)
	{
		switch (rc)
		{
		case bi::RESULT_CODE_SUCCESS: return "SUCCESS";
		case bi::RESULT_CODE_FAILURE_ANALYSIS_ERROR: return "FAILURE_ANALYSIS_ERROR";
		case bi::RESULT_CODE_FAILURE_CANCELLED: return "FAILURE_CANCELLED";
		case bi::RESULT_CODE_FAILURE_INVALID_INPUT_LOG_FILE: return "FAILURE_INVALID_INPUT_LOG_FILE";
		case bi::RESULT_CODE_FAILURE_INVALID_OUTPUT_LOG_FILE: return "FAILURE_INVALID_OUTPUT_LOG_FILE";
		case bi::RESULT_CODE_FAILURE_OPEN_INPUT_TRACE: return "FAILURE_OPEN_INPUT_TRACE";
		case bi::RESULT_CODE_FAILURE_PROCESS_TRACE: return "FAILURE_PROCESS_TRACE";
		case bi::RESULT_CODE_FAILURE_DROPPED_EVENTS: return "FAILURE_DROPPED_EVENTS";
		case bi::RESULT_CODE_FAILURE_UNSUPPORTED_OS: return "FAILURE_UNSUPPORTED_OS";
		case bi::RESULT_CODE_FAILURE_INVALID_TRACING_SESSION_NAME: return "FAILURE_INVALID_TRACING_SESSION_NAME";
		case bi::RESULT_CODE_FAILURE_INSUFFICIENT_PRIVILEGES: return "FAILURE_INSUFFICIENT_PRIVILEGES (管理者権限が要る)";
		case bi::RESULT_CODE_FAILURE_START_SYSTEM_TRACE: return "FAILURE_START_SYSTEM_TRACE";
		case bi::RESULT_CODE_FAILURE_START_MSVC_TRACE: return "FAILURE_START_MSVC_TRACE (同名のセッションが残っている可能性)";
		case bi::RESULT_CODE_FAILURE_STOP_MSVC_TRACE: return "FAILURE_STOP_MSVC_TRACE";
		case bi::RESULT_CODE_FAILURE_STOP_SYSTEM_TRACE: return "FAILURE_STOP_SYSTEM_TRACE";
		case bi::RESULT_CODE_FAILURE_MSVC_TRACE_FILE_NOT_FOUND: return "FAILURE_MSVC_TRACE_FILE_NOT_FOUND";
		case bi::RESULT_CODE_FAILURE_MERGE_TRACES: return "FAILURE_MERGE_TRACES";
		default: return "FAILURE (その他)";
		}
	}

	// ------------------------------------------------------------
	// JSON 出力
	// ------------------------------------------------------------

	class JsonWriter
	{
	public:
		explicit JsonWriter(std::string& out) : out_(out) {}

		void Raw(const char* s) { out_ += s; }
		void Raw(const std::string& s) { out_ += s; }
		void Int(long long v) { out_ += std::to_string(v); }
		void Bool(bool v) { out_ += v ? "true" : "false"; }

		void Str(const std::string& s)
		{
			out_ += '"';
			for (const char ch : s)
			{
				const auto c = static_cast<unsigned char>(ch);
				switch (c)
				{
				case '"': out_ += "\\\""; break;
				case '\\': out_ += "\\\\"; break;
				case '\n': out_ += "\\n"; break;
				case '\r': out_ += "\\r"; break;
				case '\t': out_ += "\\t"; break;
				default:
					if (c < 0x20)
					{
						char buf[8];
						std::snprintf(buf, sizeof(buf), "\\u%04x", c);
						out_ += buf;
					}
					else
					{
						out_ += ch;
					}
					break;
				}
			}
			out_ += '"';
		}

		void Key(const char* k)
		{
			Str(k);
			out_ += ':';
		}

	private:
		std::string& out_;
	};

	// ------------------------------------------------------------
	// 集計用のデータ
	// ------------------------------------------------------------

	struct FileNode
	{
		uint32_t path = 0;
		int32_t parent = -1;
		long long incl_us = 0;
		long long excl_us = 0;
		long long wctr_us = 0;
	};

	struct TemplateLocal
	{
		long long incl_us = 0;
		long long excl_us = 0;
		long long count = 0;
		int kind = 0;
	};

	struct SpecializationLocal
	{
		unsigned long long primary = 0;
		long long incl_us = 0;
		long long count = 0;
	};

	struct PassData
	{
		unsigned invocation = 0;
		std::string source;
		std::string object;
		long long start_us = 0;
		long long fe_us = 0;
		long long fe_wctr_us = 0;
		long long be_us = -1;
		long long be_wctr_us = -1;
		bool is_pch = false;
		bool finished = false;
		std::vector<FileNode> nodes;
		std::unordered_map<unsigned long long, int32_t> open_nodes;
		std::unordered_map<unsigned long long, TemplateLocal> templates;
		std::unordered_map<unsigned long long, SpecializationLocal> specializations;
		/// (primary key, パスの番号) → inclusive
		std::unordered_map<unsigned long long, std::unordered_map<uint32_t, long long>> template_files;
		std::unordered_map<unsigned long long, std::string> names;
	};

	struct ParentStat
	{
		long long count = 0;
		long long incl_us = 0;
	};

	struct HeaderStat
	{
		long long incl_us = 0;
		long long excl_us = 0;
		long long wctr_us = 0;
		long long passes = 0;
		long long pch_passes = 0;
		long long parses = 0;
		long long max_us = 0;
		std::unordered_map<uint32_t, ParentStat> parents;
	};

	struct SpecStat
	{
		long long incl_us = 0;
		long long count = 0;
	};

	struct TemplateStat
	{
		long long incl_us = 0;
		long long excl_us = 0;
		long long count = 0;
		long long passes = 0;
		int kind = 0;
		std::unordered_map<std::string, SpecStat> specs;
		std::unordered_map<uint32_t, long long> files;
	};

	struct FunctionStat
	{
		std::string name;
		long long dur_us = 0;
		long long wctr_us = 0;
		long long count = 0;
		long long max_us = 0;
		bool in_cl = false;
		bool in_ltcg = false;
		long long inlinees = 0;
		long long inline_size = 0;
		std::vector<std::pair<std::string, int>> top_inlinees;
	};

	struct InvocationStat
	{
		unsigned id = 0;
		bool is_linker = false;
		long long start_us = 0;
		long long dur_us = 0;
		long long wctr_us = 0;
		std::vector<std::string> outputs;
	};

	// ------------------------------------------------------------
	// 解析器
	// ------------------------------------------------------------

	class Collector : public bi::IAnalyzer
	{
	public:
		bi::AnalysisControl OnTraceInfo(const bi::TraceInfo& info) override
		{
			trace_duration_us_ = ToUs(info.Duration());
			logical_processors_ = info.LogicalProcessorCount();
			return bi::AnalysisControl::CONTINUE;
		}

		bi::AnalysisControl OnStartActivity(const bi::EventStack& stack) override
		{
			// 時刻は最初のイベントからの経過にする。TraceInfo の開始時刻はイベントの時刻と
			// 基準が違い、引くと大きな負の値になる (CI の実データで確かめた)
			if (origin_ == LLONG_MAX)
			{
				origin_ = stack.Back().StartTimestamp();
				tick_frequency_ = stack.Back().TickFrequency();
			}
			switch (stack.Back().EventId())
			{
			case bi::EVENT_ID_FRONT_END_PASS:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStartPass);
				break;
			case bi::EVENT_ID_FRONT_END_FILE:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStartFile);
				break;
			default:
				break;
			}
			return bi::AnalysisControl::CONTINUE;
		}

		bi::AnalysisControl OnStopActivity(const bi::EventStack& stack) override
		{
			switch (stack.Back().EventId())
			{
			case bi::EVENT_ID_FRONT_END_PASS:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopPass);
				break;
			case bi::EVENT_ID_BACK_END_PASS:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopBackEnd);
				break;
			case bi::EVENT_ID_FRONT_END_FILE:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopFile);
				break;
			case bi::EVENT_ID_TEMPLATE_INSTANTIATION:
				if (!bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopTemplateInFile))
				{
					bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopTemplate);
				}
				break;
			case bi::EVENT_ID_FUNCTION:
				if (!bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopFunctionInLinker))
				{
					bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopFunctionInCompiler);
				}
				break;
			case bi::EVENT_ID_COMPILER:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopCompiler);
				break;
			case bi::EVENT_ID_LINKER:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnStopLinker);
				break;
			default:
				break;
			}
			return bi::AnalysisControl::CONTINUE;
		}

		bi::AnalysisControl OnSimpleEvent(const bi::EventStack& stack) override
		{
			switch (stack.Back().EventId())
			{
			case bi::EVENT_ID_SYMBOL_NAME:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnSymbolName);
				break;
			case bi::EVENT_ID_PRECOMPILED_HEADER:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnPrecompiledHeader);
				break;
			case bi::EVENT_ID_FORCE_INLINEE:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnForceInlinee);
				break;
			case bi::EVENT_ID_EXECUTABLE_IMAGE_OUTPUT:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnImageOutput);
				break;
			case bi::EVENT_ID_LIB_OUTPUT:
				bi::MatchEventStackInMemberFunction(stack, this, &Collector::OnLibOutput);
				break;
			default:
				break;
			}
			return bi::AnalysisControl::CONTINUE;
		}

		std::string ToJson(const std::string& stats_json) const;

	private:
		long long RelUs(long long ticks) const
		{
			if (tick_frequency_ <= 0 || origin_ == LLONG_MAX)
			{
				return 0;
			}
			const long long d = ticks - origin_;
			return (d / tick_frequency_) * 1000000 + (d % tick_frequency_) * 1000000 / tick_frequency_;
		}

		uint32_t InternPath(const char* raw)
		{
			std::string path = NarrowToUtf8(raw);
			std::string key = ToLowerAscii(path);
			auto it = path_index_.find(key);
			if (it != path_index_.end())
			{
				return it->second;
			}
			const auto index = static_cast<uint32_t>(paths_.size());
			paths_.push_back(std::move(path));
			path_index_.emplace(std::move(key), index);
			return index;
		}

		static std::string PassKey(unsigned invocation, const wchar_t* source)
		{
			return std::to_string(invocation) + "|" + ToLowerAscii(WideToUtf8(source));
		}

		PassData* FindPass(unsigned long long id)
		{
			auto it = passes_.find(id);
			return it == passes_.end() ? nullptr : &it->second;
		}

		void OnStartPass(act::Compiler cl, act::FrontEndPass fe)
		{
			PassData& pass = passes_[fe.EventInstanceId()];
			pass.invocation = cl.InvocationId();
			pass.source = WideToUtf8(fe.InputSourcePath());
			pass.object = WideToUtf8(fe.OutputObjectPath());
			pass.start_us = RelUs(fe.StartTimestamp());
			pass_order_.push_back(fe.EventInstanceId());
			pass_by_source_[PassKey(cl.InvocationId(), fe.InputSourcePath())] = fe.EventInstanceId();
		}

		void OnStartFile(act::FrontEndPass fe, act::FrontEndFileGroup files)
		{
			PassData* pass = FindPass(fe.EventInstanceId());
			if (pass == nullptr)
			{
				return;
			}
			FileNode node;
			node.path = InternPath(files.Back().Path());
			if (files.Size() > 1)
			{
				auto parent = pass->open_nodes.find(files[files.Size() - 2].EventInstanceId());
				if (parent != pass->open_nodes.end())
				{
					node.parent = parent->second;
				}
			}
			pass->open_nodes[files.Back().EventInstanceId()] = static_cast<int32_t>(pass->nodes.size());
			pass->nodes.push_back(node);
		}

		void OnStopFile(act::FrontEndPass fe, act::FrontEndFile file)
		{
			PassData* pass = FindPass(fe.EventInstanceId());
			if (pass == nullptr)
			{
				return;
			}
			auto it = pass->open_nodes.find(file.EventInstanceId());
			if (it == pass->open_nodes.end())
			{
				return;
			}
			FileNode& node = pass->nodes[static_cast<size_t>(it->second)];
			node.incl_us = ToUs(file.Duration());
			node.excl_us = ToUs(file.ExclusiveDuration());
			node.wctr_us = ToUs(file.WallClockTimeResponsibility());
			pass->open_nodes.erase(it);
			++file_parses_;
		}

		void RecordTemplate(act::FrontEndPass fe, const char* file_path, const act::TemplateInstantiationGroup& group)
		{
			PassData* pass = FindPass(fe.EventInstanceId());
			if (pass == nullptr)
			{
				return;
			}
			const act::TemplateInstantiation& ti = group.Back();
			const unsigned long long primary = ti.PrimaryTemplateSymbolKey();
			bool nested_same = false;
			for (size_t i = 0; i + 1 < group.Size(); ++i)
			{
				if (group[i].PrimaryTemplateSymbolKey() == primary)
				{
					nested_same = true;
					break;
				}
			}
			const long long incl = ToUs(ti.Duration());
			TemplateLocal& t = pass->templates[primary];
			t.excl_us += ToUs(ti.ExclusiveDuration());
			t.count += 1;
			t.kind = static_cast<int>(ti.Kind());
			SpecializationLocal& s = pass->specializations[ti.SpecializationSymbolKey()];
			s.primary = primary;
			s.count += 1;
			if (!nested_same)
			{
				t.incl_us += incl;
				s.incl_us += incl;
				if (file_path != nullptr)
				{
					pass->template_files[primary][InternPath(file_path)] += incl;
				}
			}
			++template_instantiations_;
		}

		void OnStopTemplateInFile(act::FrontEndPass fe, act::FrontEndFile file, act::TemplateInstantiationGroup group)
		{
			RecordTemplate(fe, file.Path(), group);
		}

		void OnStopTemplate(act::FrontEndPass fe, act::TemplateInstantiationGroup group)
		{
			RecordTemplate(fe, nullptr, group);
		}

		void OnSymbolName(act::FrontEndPass fe, se::SymbolName symbol)
		{
			PassData* pass = FindPass(fe.EventInstanceId());
			if (pass == nullptr)
			{
				return;
			}
			const unsigned long long key = symbol.Key();
			if (pass->templates.count(key) != 0 || pass->specializations.count(key) != 0)
			{
				pass->names[key] = NarrowToUtf8(symbol.Name());
			}
		}

		void OnPrecompiledHeader(act::FrontEndPass fe, se::PrecompiledHeader)
		{
			PassData* pass = FindPass(fe.EventInstanceId());
			if (pass != nullptr)
			{
				pass->is_pch = true;
			}
		}

		void OnStopPass(act::FrontEndPass fe)
		{
			PassData* pass = FindPass(fe.EventInstanceId());
			if (pass == nullptr)
			{
				return;
			}
			pass->fe_us = ToUs(fe.Duration());
			pass->fe_wctr_us = ToUs(fe.WallClockTimeResponsibility());
			pass->finished = true;
			total_fe_us_ += pass->fe_us;
			total_fe_wctr_us_ += pass->fe_wctr_us;
			MergeHeaders(*pass);
			MergeTemplates(*pass);
			pass->open_nodes.clear();
		}

		void MergeHeaders(const PassData& pass)
		{
			std::unordered_set<uint32_t> seen;
			for (const FileNode& node : pass.nodes)
			{
				// 根 (翻訳単位のソース自身) はヘッダではない
				if (node.parent < 0)
				{
					continue;
				}
				HeaderStat& h = headers_[node.path];
				h.incl_us += node.incl_us;
				h.excl_us += node.excl_us;
				h.wctr_us += node.wctr_us;
				h.parses += 1;
				h.max_us = std::max(h.max_us, node.incl_us);
				if (seen.insert(node.path).second)
				{
					h.passes += 1;
					if (pass.is_pch)
					{
						h.pch_passes += 1;
					}
				}
				ParentStat& p = h.parents[pass.nodes[static_cast<size_t>(node.parent)].path];
				p.count += 1;
				p.incl_us += node.incl_us;
			}
		}

		void MergeTemplates(PassData& pass)
		{
			const auto name_of = [&pass](unsigned long long key) -> std::string
			{
				auto it = pass.names.find(key);
				return it == pass.names.end() ? std::string("<unknown>") : it->second;
			};
			std::unordered_map<unsigned long long, TemplateStat*> resolved;
			for (const auto& [key, local] : pass.templates)
			{
				TemplateStat& t = templates_[name_of(key)];
				t.incl_us += local.incl_us;
				t.excl_us += local.excl_us;
				t.count += local.count;
				t.passes += 1;
				t.kind = local.kind;
				resolved[key] = &t;
			}
			for (const auto& [key, local] : pass.specializations)
			{
				auto it = resolved.find(local.primary);
				if (it == resolved.end())
				{
					continue;
				}
				SpecStat& s = it->second->specs[name_of(key)];
				s.incl_us += local.incl_us;
				s.count += local.count;
			}
			for (const auto& [key, files] : pass.template_files)
			{
				auto it = resolved.find(key);
				if (it == resolved.end())
				{
					continue;
				}
				for (const auto& [path, incl] : files)
				{
					it->second->files[path] += incl;
				}
			}
			pass.templates.clear();
			pass.specializations.clear();
			pass.template_files.clear();
			pass.names.clear();
		}

		void OnStopBackEnd(act::Compiler cl, act::BackEndPass be)
		{
			total_be_us_ += ToUs(be.Duration());
			total_be_wctr_us_ += ToUs(be.WallClockTimeResponsibility());
			auto it = pass_by_source_.find(PassKey(cl.InvocationId(), be.InputSourcePath()));
			if (it == pass_by_source_.end())
			{
				return;
			}
			PassData* pass = FindPass(it->second);
			if (pass != nullptr)
			{
				pass->be_us = ToUs(be.Duration());
				pass->be_wctr_us = ToUs(be.WallClockTimeResponsibility());
			}
		}

		void RecordFunction(const act::Function& f, bool ltcg)
		{
			const std::string decorated = NarrowToUtf8(f.Name());
			FunctionStat& s = functions_[decorated];
			const long long dur = ToUs(f.Duration());
			s.dur_us += dur;
			s.wctr_us += ToUs(f.WallClockTimeResponsibility());
			s.count += 1;
			s.max_us = std::max(s.max_us, dur);
			(ltcg ? s.in_ltcg : s.in_cl) = true;
			auto it = pending_inlinees_.find(f.EventInstanceId());
			if (it != pending_inlinees_.end())
			{
				for (auto& inlinee : it->second)
				{
					s.inlinees += 1;
					s.inline_size += inlinee.second;
					auto found = std::find_if(s.top_inlinees.begin(), s.top_inlinees.end(),
						[&inlinee](const auto& e) { return e.first == inlinee.first; });
					if (found != s.top_inlinees.end())
					{
						found->second = std::max(found->second, inlinee.second);
					}
					else
					{
						s.top_inlinees.push_back(std::move(inlinee));
					}
				}
				std::sort(s.top_inlinees.begin(), s.top_inlinees.end(),
					[](const auto& a, const auto& b) { return a.second > b.second; });
				if (s.top_inlinees.size() > kMaxInlinees)
				{
					s.top_inlinees.resize(kMaxInlinees);
				}
				pending_inlinees_.erase(it);
			}
			++function_count_;
		}

		void OnStopFunctionInLinker(act::Linker, act::Function f)
		{
			RecordFunction(f, true);
		}

		void OnStopFunctionInCompiler(act::Compiler, act::Function f)
		{
			RecordFunction(f, false);
		}

		void OnForceInlinee(act::Function f, se::ForceInlinee inlinee)
		{
			pending_inlinees_[f.EventInstanceId()].emplace_back(NarrowToUtf8(inlinee.Name()), static_cast<int>(inlinee.Size()));
		}

		void RecordInvocation(const act::Invocation& inv, bool linker)
		{
			InvocationStat& s = invocations_[inv.EventInstanceId()];
			s.id = inv.InvocationId();
			s.is_linker = linker;
			s.start_us = RelUs(inv.StartTimestamp());
			s.dur_us = ToUs(inv.Duration());
			s.wctr_us = ToUs(inv.WallClockTimeResponsibility());
			build_start_ = std::min(build_start_, inv.StartTimestamp());
			build_stop_ = std::max(build_stop_, inv.StopTimestamp());
		}

		void OnStopCompiler(act::Compiler cl)
		{
			RecordInvocation(cl, false);
		}

		void OnStopLinker(act::Linker link)
		{
			RecordInvocation(link, true);
		}

		void OnImageOutput(act::Linker link, se::ExecutableImageOutput output)
		{
			invocations_[link.EventInstanceId()].outputs.push_back(WideToUtf8(output.Path()));
		}

		void OnLibOutput(act::Linker link, se::LibOutput output)
		{
			invocations_[link.EventInstanceId()].outputs.push_back(WideToUtf8(output.Path()));
		}

		void WriteTree(JsonWriter& w, const PassData& pass, const std::vector<std::vector<int32_t>>& children, int32_t index) const;

		long long origin_ = LLONG_MAX;
		long long tick_frequency_ = 0;
		long long trace_duration_us_ = 0;
		unsigned long logical_processors_ = 0;
		long long build_start_ = LLONG_MAX;
		long long build_stop_ = LLONG_MIN;

		long long total_fe_us_ = 0;
		long long total_fe_wctr_us_ = 0;
		long long total_be_us_ = 0;
		long long total_be_wctr_us_ = 0;
		long long template_instantiations_ = 0;
		long long function_count_ = 0;
		long long file_parses_ = 0;

		std::vector<std::string> paths_;
		std::unordered_map<std::string, uint32_t> path_index_;
		std::unordered_map<unsigned long long, PassData> passes_;
		std::vector<unsigned long long> pass_order_;
		std::unordered_map<std::string, unsigned long long> pass_by_source_;
		std::unordered_map<uint32_t, HeaderStat> headers_;
		std::unordered_map<std::string, TemplateStat> templates_;
		std::unordered_map<std::string, FunctionStat> functions_;
		std::unordered_map<unsigned long long, std::vector<std::pair<std::string, int>>> pending_inlinees_;
		std::unordered_map<unsigned long long, InvocationStat> invocations_;
	};

	/// ノードは [パスの番号, inclusive, exclusive, wctr, [子...], 畳んだ数, 畳んだ inclusive の合計]
	void Collector::WriteTree(JsonWriter& w, const PassData& pass, const std::vector<std::vector<int32_t>>& children, int32_t index) const
	{
		const FileNode& node = pass.nodes[static_cast<size_t>(index)];
		w.Raw("[");
		w.Int(node.path);
		w.Raw(",");
		w.Int(node.incl_us);
		w.Raw(",");
		w.Int(node.excl_us);
		w.Raw(",");
		w.Int(node.wctr_us);
		w.Raw(",[");
		long long folded = 0;
		long long folded_us = 0;
		bool first = true;
		for (const int32_t child : children[static_cast<size_t>(index)])
		{
			const FileNode& c = pass.nodes[static_cast<size_t>(child)];
			if (c.incl_us < kTreeThresholdUs)
			{
				++folded;
				folded_us += c.incl_us;
				continue;
			}
			if (!first)
			{
				w.Raw(",");
			}
			first = false;
			WriteTree(w, pass, children, child);
		}
		w.Raw("],");
		w.Int(folded);
		w.Raw(",");
		w.Int(folded_us);
		w.Raw("]");
	}

	std::string Collector::ToJson(const std::string& stats_json) const
	{
		std::string out;
		out.reserve(8 * 1024 * 1024);
		JsonWriter w(out);
		w.Raw("{");
		w.Key("format");
		w.Int(1);
		w.Raw(",");
		w.Key("stats");
		w.Raw(stats_json.empty() ? std::string("null") : stats_json);
		w.Raw(",");

		w.Key("trace");
		w.Raw("{");
		w.Key("duration_us");
		w.Int(trace_duration_us_);
		w.Raw(",");
		w.Key("logical_processors");
		w.Int(static_cast<long long>(logical_processors_));
		w.Raw("},");

		w.Key("build");
		w.Raw("{");
		const bool has_build = build_start_ <= build_stop_;
		w.Key("start_us");
		w.Int(has_build ? RelUs(build_start_) : 0);
		w.Raw(",");
		w.Key("end_us");
		w.Int(has_build ? RelUs(build_stop_) : 0);
		w.Raw("},");

		w.Key("totals");
		w.Raw("{");
		w.Key("fe_us");
		w.Int(total_fe_us_);
		w.Raw(",");
		w.Key("fe_wctr_us");
		w.Int(total_fe_wctr_us_);
		w.Raw(",");
		w.Key("be_us");
		w.Int(total_be_us_);
		w.Raw(",");
		w.Key("be_wctr_us");
		w.Int(total_be_wctr_us_);
		w.Raw(",");
		w.Key("template_instantiations");
		w.Int(template_instantiations_);
		w.Raw(",");
		w.Key("functions");
		w.Int(function_count_);
		w.Raw(",");
		w.Key("file_parses");
		w.Int(file_parses_);
		w.Raw(",");
		w.Key("passes");
		w.Int(static_cast<long long>(pass_order_.size()));
		w.Raw("},");

		// 起動順に並べる
		std::vector<const InvocationStat*> invocations;
		for (const auto& [id, inv] : invocations_)
		{
			if (inv.dur_us > 0)
			{
				invocations.push_back(&inv);
			}
		}
		std::sort(invocations.begin(), invocations.end(),
			[](const InvocationStat* a, const InvocationStat* b) { return a->start_us < b->start_us; });
		w.Key("invocations");
		w.Raw("[");
		for (size_t i = 0; i < invocations.size(); ++i)
		{
			const InvocationStat& inv = *invocations[i];
			if (i != 0)
			{
				w.Raw(",");
			}
			w.Raw("{");
			w.Key("id");
			w.Int(inv.id);
			w.Raw(",");
			w.Key("type");
			w.Str(inv.is_linker ? "link" : "cl");
			w.Raw(",");
			w.Key("start_us");
			w.Int(inv.start_us);
			w.Raw(",");
			w.Key("dur_us");
			w.Int(inv.dur_us);
			w.Raw(",");
			w.Key("wctr_us");
			w.Int(inv.wctr_us);
			w.Raw(",");
			w.Key("outputs");
			w.Raw("[");
			for (size_t j = 0; j < inv.outputs.size(); ++j)
			{
				if (j != 0)
				{
					w.Raw(",");
				}
				w.Str(inv.outputs[j]);
			}
			w.Raw("]}");
		}
		w.Raw("],");

		w.Key("paths");
		w.Raw("[");
		for (size_t i = 0; i < paths_.size(); ++i)
		{
			if (i != 0)
			{
				w.Raw(",");
			}
			w.Str(paths_[i]);
		}
		w.Raw("],");

		w.Key("units");
		w.Raw("[");
		bool first_unit = true;
		for (const unsigned long long id : pass_order_)
		{
			const PassData& pass = passes_.at(id);
			if (!pass.finished)
			{
				continue;
			}
			if (!first_unit)
			{
				w.Raw(",");
			}
			first_unit = false;
			w.Raw("{");
			w.Key("source");
			w.Str(pass.source);
			w.Raw(",");
			w.Key("object");
			w.Str(pass.object);
			w.Raw(",");
			w.Key("inv");
			w.Int(pass.invocation);
			w.Raw(",");
			w.Key("start_us");
			w.Int(pass.start_us);
			w.Raw(",");
			w.Key("fe_us");
			w.Int(pass.fe_us);
			w.Raw(",");
			w.Key("fe_wctr_us");
			w.Int(pass.fe_wctr_us);
			w.Raw(",");
			w.Key("be_us");
			w.Int(pass.be_us);
			w.Raw(",");
			w.Key("be_wctr_us");
			w.Int(pass.be_wctr_us);
			w.Raw(",");
			w.Key("pch");
			w.Bool(pass.is_pch);
			w.Raw(",");
			w.Key("parses");
			w.Int(static_cast<long long>(pass.nodes.size()));
			w.Raw(",");
			w.Key("tree");
			std::vector<std::vector<int32_t>> children(pass.nodes.size());
			int32_t root = -1;
			for (size_t i = 0; i < pass.nodes.size(); ++i)
			{
				const int32_t parent = pass.nodes[i].parent;
				if (parent >= 0)
				{
					children[static_cast<size_t>(parent)].push_back(static_cast<int32_t>(i));
				}
				else if (root < 0)
				{
					root = static_cast<int32_t>(i);
				}
			}
			if (root >= 0)
			{
				WriteTree(w, pass, children, root);
			}
			else
			{
				w.Raw("null");
			}
			w.Raw("}");
		}
		w.Raw("],");

		w.Key("headers");
		w.Raw("[");
		bool first_header = true;
		for (const auto& [path, h] : headers_)
		{
			if (!first_header)
			{
				w.Raw(",");
			}
			first_header = false;
			w.Raw("{");
			w.Key("p");
			w.Int(path);
			w.Raw(",");
			w.Key("incl_us");
			w.Int(h.incl_us);
			w.Raw(",");
			w.Key("excl_us");
			w.Int(h.excl_us);
			w.Raw(",");
			w.Key("wctr_us");
			w.Int(h.wctr_us);
			w.Raw(",");
			w.Key("passes");
			w.Int(h.passes);
			w.Raw(",");
			w.Key("pch_passes");
			w.Int(h.pch_passes);
			w.Raw(",");
			w.Key("parses");
			w.Int(h.parses);
			w.Raw(",");
			w.Key("max_us");
			w.Int(h.max_us);
			w.Raw(",");
			w.Key("parents");
			std::vector<std::pair<uint32_t, ParentStat>> parents(h.parents.begin(), h.parents.end());
			std::sort(parents.begin(), parents.end(),
				[](const auto& a, const auto& b) { return a.second.incl_us > b.second.incl_us; });
			if (parents.size() > kMaxParents)
			{
				parents.resize(kMaxParents);
			}
			w.Raw("[");
			for (size_t i = 0; i < parents.size(); ++i)
			{
				if (i != 0)
				{
					w.Raw(",");
				}
				w.Raw("[");
				w.Int(parents[i].first);
				w.Raw(",");
				w.Int(parents[i].second.count);
				w.Raw(",");
				w.Int(parents[i].second.incl_us);
				w.Raw("]");
			}
			w.Raw("]}");
		}
		w.Raw("],");

		std::vector<std::pair<const std::string*, const TemplateStat*>> templates;
		for (const auto& [name, t] : templates_)
		{
			templates.emplace_back(&name, &t);
		}
		std::sort(templates.begin(), templates.end(),
			[](const auto& a, const auto& b) { return a.second->incl_us > b.second->incl_us; });
		if (templates.size() > kMaxTemplates)
		{
			templates.resize(kMaxTemplates);
		}
		w.Key("templates");
		w.Raw("[");
		for (size_t i = 0; i < templates.size(); ++i)
		{
			const TemplateStat& t = *templates[i].second;
			if (i != 0)
			{
				w.Raw(",");
			}
			w.Raw("{");
			w.Key("name");
			w.Str(*templates[i].first);
			w.Raw(",");
			w.Key("kind");
			w.Int(t.kind);
			w.Raw(",");
			w.Key("incl_us");
			w.Int(t.incl_us);
			w.Raw(",");
			w.Key("excl_us");
			w.Int(t.excl_us);
			w.Raw(",");
			w.Key("count");
			w.Int(t.count);
			w.Raw(",");
			w.Key("passes");
			w.Int(t.passes);
			w.Raw(",");
			w.Key("specs");
			std::vector<std::pair<const std::string*, SpecStat>> specs;
			for (const auto& [name, s] : t.specs)
			{
				specs.emplace_back(&name, s);
			}
			std::sort(specs.begin(), specs.end(),
				[](const auto& a, const auto& b) { return a.second.incl_us > b.second.incl_us; });
			if (specs.size() > kMaxSpecializations)
			{
				specs.resize(kMaxSpecializations);
			}
			w.Raw("[");
			for (size_t j = 0; j < specs.size(); ++j)
			{
				if (j != 0)
				{
					w.Raw(",");
				}
				w.Raw("[");
				w.Str(*specs[j].first);
				w.Raw(",");
				w.Int(specs[j].second.incl_us);
				w.Raw(",");
				w.Int(specs[j].second.count);
				w.Raw("]");
			}
			w.Raw("],");
			w.Key("spec_count");
			w.Int(static_cast<long long>(t.specs.size()));
			w.Raw(",");
			w.Key("files");
			std::vector<std::pair<uint32_t, long long>> files(t.files.begin(), t.files.end());
			std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
			if (files.size() > kMaxTemplateFiles)
			{
				files.resize(kMaxTemplateFiles);
			}
			w.Raw("[");
			for (size_t j = 0; j < files.size(); ++j)
			{
				if (j != 0)
				{
					w.Raw(",");
				}
				w.Raw("[");
				w.Int(files[j].first);
				w.Raw(",");
				w.Int(files[j].second);
				w.Raw("]");
			}
			w.Raw("]}");
		}
		w.Raw("],");

		std::vector<std::pair<const std::string*, const FunctionStat*>> functions;
		for (const auto& [name, f] : functions_)
		{
			functions.emplace_back(&name, &f);
		}
		std::sort(functions.begin(), functions.end(),
			[](const auto& a, const auto& b) { return a.second->dur_us > b.second->dur_us; });
		if (functions.size() > kMaxFunctions)
		{
			functions.resize(kMaxFunctions);
		}
		w.Key("functions");
		w.Raw("[");
		for (size_t i = 0; i < functions.size(); ++i)
		{
			const FunctionStat& f = *functions[i].second;
			if (i != 0)
			{
				w.Raw(",");
			}
			w.Raw("{");
			w.Key("name");
			w.Str(Undecorate(*functions[i].first));
			w.Raw(",");
			w.Key("decorated");
			w.Str(*functions[i].first);
			w.Raw(",");
			w.Key("dur_us");
			w.Int(f.dur_us);
			w.Raw(",");
			w.Key("wctr_us");
			w.Int(f.wctr_us);
			w.Raw(",");
			w.Key("count");
			w.Int(f.count);
			w.Raw(",");
			w.Key("max_us");
			w.Int(f.max_us);
			w.Raw(",");
			w.Key("where");
			w.Str(f.in_cl && f.in_ltcg ? "both" : (f.in_ltcg ? "ltcg" : "cl"));
			w.Raw(",");
			w.Key("inlinees");
			w.Int(f.inlinees);
			w.Raw(",");
			w.Key("inline_size");
			w.Int(f.inline_size);
			w.Raw(",");
			w.Key("top_inlinees");
			w.Raw("[");
			for (size_t j = 0; j < f.top_inlinees.size(); ++j)
			{
				if (j != 0)
				{
					w.Raw(",");
				}
				w.Raw("[");
				w.Str(Undecorate(f.top_inlinees[j].first));
				w.Raw(",");
				w.Int(f.top_inlinees[j].second);
				w.Raw("]");
			}
			w.Raw("]}");
		}
		w.Raw("]}");
		out += '\n';
		return out;
	}

	// ------------------------------------------------------------
	// コマンド
	// ------------------------------------------------------------

	bool ReadWholeFile(const std::wstring& path, std::string& out)
	{
		FILE* f = nullptr;
		if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || f == nullptr)
		{
			return false;
		}
		char buffer[4096];
		size_t n = 0;
		while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0)
		{
			out.append(buffer, n);
		}
		std::fclose(f);
		return true;
	}

	bool WriteWholeFile(const std::wstring& path, const std::string& data)
	{
		FILE* f = nullptr;
		if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || f == nullptr)
		{
			return false;
		}
		const size_t written = std::fwrite(data.data(), 1, data.size(), f);
		std::fclose(f);
		return written == data.size();
	}

	std::wstring StatsPath(const std::wstring& raw)
	{
		return raw + L".stats.json";
	}

	int Start(const std::wstring& session, bool templates)
	{
		// 打ち切られた前回の実行がセッションを残していると start が失敗するので、先に止めて捨てる。
		// 残っていなければ失敗するだけなので結果は見ない。
		{
			wchar_t temp_dir[MAX_PATH];
			const DWORD n = GetTempPathW(MAX_PATH, temp_dir);
			if (n > 0 && n < MAX_PATH)
			{
				const std::wstring leftover = std::wstring(temp_dir) + L"nox_build_insights_leftover.etl";
				bi::TRACING_SESSION_STATISTICS ignored{};
				if (bi::StopTracingSession(session.c_str(), leftover.c_str(), &ignored) == bi::RESULT_CODE_SUCCESS)
				{
					std::printf("前回のセッションが残っていたので止めた\n");
				}
				DeleteFileW(leftover.c_str());
			}
		}

		bi::TRACING_SESSION_OPTIONS options{};
		// system event は CPU サンプリングだけ。集計に使わないので取らない (トレースが小さく済む)。
		// FAILURE_START_SYSTEM_TRACE が出るならここを疑う
		options.SystemEventFlags = 0;
		options.MsvcEventFlags = bi::TRACING_SESSION_MSVC_EVENT_FLAGS_BASIC |
			bi::TRACING_SESSION_MSVC_EVENT_FLAGS_FRONTEND_FILES |
			bi::TRACING_SESSION_MSVC_EVENT_FLAGS_BACKEND_FUNCTIONS;
		if (templates)
		{
			options.MsvcEventFlags |= bi::TRACING_SESSION_MSVC_EVENT_FLAGS_FRONTEND_TEMPLATE_INSTANTIATIONS;
		}
		const bi::RESULT_CODE rc = bi::StartTracingSession(session.c_str(), options);
		if (rc != bi::RESULT_CODE_SUCCESS)
		{
			std::fprintf(stderr, "計測を開始できなかった: %s (%d)\n", ResultCodeName(rc), static_cast<int>(rc));
			return 1;
		}
		std::printf("計測を開始した (テンプレート: %s)\n", templates ? "あり" : "なし");
		return 0;
	}

	int Stop(const std::wstring& session, const std::wstring& raw)
	{
		bi::TRACING_SESSION_STATISTICS stats{};
		const bi::RESULT_CODE rc = bi::StopTracingSession(session.c_str(), raw.c_str(), &stats);
		std::printf("欠落した MSVC イベント: %lu (バッファ %lu)\n", stats.MSVCEventsLost, stats.MSVCBuffersLost);
		std::string json;
		JsonWriter w(json);
		w.Raw("{");
		w.Key("result");
		w.Str(ResultCodeName(rc));
		w.Raw(",");
		w.Key("msvc_events_lost");
		w.Int(stats.MSVCEventsLost);
		w.Raw(",");
		w.Key("msvc_buffers_lost");
		w.Int(stats.MSVCBuffersLost);
		w.Raw(",");
		w.Key("system_events_lost");
		w.Int(stats.SystemEventsLost);
		w.Raw(",");
		w.Key("system_buffers_lost");
		w.Int(stats.SystemBuffersLost);
		w.Raw("}");
		WriteWholeFile(StatsPath(raw), json);
		// イベントが欠けても ETL は書かれるので、集計は続けられる (欠落は JSON に残して警告する)
		if (rc != bi::RESULT_CODE_SUCCESS && rc != bi::RESULT_CODE_FAILURE_DROPPED_EVENTS)
		{
			std::fprintf(stderr, "計測を止められなかった: %s (%d)\n", ResultCodeName(rc), static_cast<int>(rc));
			return 1;
		}
		std::printf("計測を止めた\n");
		return 0;
	}

	int Analyze(const std::wstring& raw, const std::wstring& out_path)
	{
		Collector collector;
		auto group = bi::MakeStaticAnalyzerGroup(&collector);
		const bi::RESULT_CODE rc = bi::Analyze(raw.c_str(), 1, group);
		if (rc != bi::RESULT_CODE_SUCCESS)
		{
			std::fprintf(stderr, "解析に失敗した: %s (%d)\n", ResultCodeName(rc), static_cast<int>(rc));
			return 1;
		}
		std::string stats;
		ReadWholeFile(StatsPath(raw), stats);
		if (!WriteWholeFile(out_path, collector.ToJson(stats)))
		{
			std::fprintf(stderr, "出力を書けなかった\n");
			return 1;
		}
		std::printf("集計を書き出した\n");
		return 0;
	}

	int Usage()
	{
		std::fprintf(stderr,
			"usage:\n"
			"  nox_build_insights start <session> [--no-templates]\n"
			"  nox_build_insights stop <session> <raw.etl>\n"
			"  nox_build_insights analyze <raw.etl> <out.json>\n");
		return 2;
	}
}

int wmain(int argc, wchar_t** argv)
{
	SetConsoleOutputCP(CP_UTF8);
	if (argc < 3)
	{
		return Usage();
	}
	const std::wstring command = argv[1];
	if (command == L"start")
	{
		const bool templates = !(argc >= 4 && std::wstring(argv[3]) == L"--no-templates");
		return Start(argv[2], templates);
	}
	if (command == L"stop" && argc >= 4)
	{
		return Stop(argv[2], argv[3]);
	}
	if (command == L"analyze" && argc >= 4)
	{
		return Analyze(argv[2], argv[3]);
	}
	return Usage();
}
