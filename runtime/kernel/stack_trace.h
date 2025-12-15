//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	stack_trace.h
///	@brief	stack_trace
#pragma once

#include	"stack_trace_definition.h"
#include	"nox_string.h"
#include	"nox_string_view.h"

#include	"assertion.h"

namespace nox::stack_walker
{
	/// @brief スタックフレーム情報
	struct StackFrameInfo
	{
		/// @brief 行番号
		uint16 line_;

		/// @brief モジュール名
		std::array<nox::char16, nox::stack_walker::detail::kMaxModuleName> module_name_;

		/// @brief ファイル名
		std::array<nox::char16, nox::stack_walker::detail::kMaxFileName> file_name_;

		/// @brief シンボル名
		std::array<nox::char16, nox::stack_walker::detail::kMaxSymbolName> symbol_name_;
	};

	/// @brief スタック情報
	class StackFrame
	{
	public:
		inline constexpr StackFrame()noexcept :
			address_(0),
			module_name_{ u'\000' },
			file_name_{ u'\000' },
			symbol_name_{ u'\000' },
			line_(0),
			is_resolver_(false)
		{
		}

		inline consteval StackFrame(const StackFrame&)noexcept = delete;
		inline consteval StackFrame(StackFrame&&)noexcept = delete;

		inline constexpr ~StackFrame() = default;

		inline	void	SetAddress(const std::size_t address)noexcept { address_ = address; }
		inline	void	SetResolved(bool isResolver)noexcept { is_resolver_ = isResolver; }

		/// @brief アドレスを解決
		void	Resolve();

		void	SetModuleName(std::u16string_view name);
		void	SetFileName(std::u16string_view name);
		void	SetSymbolName(std::u16string_view name);
		inline	void	SetLine(const nox::uint32 line)noexcept { line_ = line; }

		[[nodiscard]]	inline	constexpr	std::size_t	GetAddress()const noexcept { return address_; }
		[[nodiscard]]	inline	constexpr	std::u16string_view GetModuleName()const noexcept { return module_name_.data(); }
		[[nodiscard]]	inline	constexpr	std::u16string_view GetSymbolName()const noexcept { return symbol_name_.data(); }
		[[nodiscard]]	inline	constexpr	std::u16string_view GetFileName()const noexcept { return file_name_.data(); }
		[[nodiscard]]	inline	constexpr	nox::uint32	GetLine()const noexcept { return line_; }

		/// @brief リゾルブ済みか
		/// @return リゾルブ済みか
		[[nodiscard]] inline	constexpr	bool	IsResolved()const noexcept { return is_resolver_; }

		/// @brief 無効なアドレスかどうかを取得
		/// @return 無効なアドレスかどうか
		[[nodiscard]] inline	constexpr	bool	IsInvalidAddress()const noexcept { return address_ == 0U; }
	private:
		/// @brief アドレス
		std::size_t address_;

		//	サイズ節約のため、char16を使用

		/// @brief モジュール名
		std::array<nox::char16, 255> module_name_;

		/// @brief ファイル名
		std::array<nox::char16, 1024> file_name_;

		/// @brief シンボル名
		std::array<nox::char16, 255> symbol_name_;

		/// @brief 行番号
		nox::uint32 line_;

		/// @brief 解決済み
		bool	is_resolver_;
	};

	/// @brief		軽量版スタック情報
	/// @details	フレームポインタのみを保持
	///				ファイルパスなどは動的に解決する
	class SlimStackFrame
	{
	public:
		inline void SetAddress(std::size_t address)noexcept { address_ = address; }

		std::span<char16, 246> ResolveModuleName()const;
		std::u16string_view ResolveModuleName(std::span<char16> dest_buffer)const;
	private:
		/// @brief アドレス
		std::size_t address_;

		/// @brief 行番号
		uint32 line_:31;

		/// @brief 解決済み
		bool	is_resolver_:1;
	};

	/**
	 * @brief stack_walkerの詳細
	*/
	namespace detail
	{
		/**
		 * @brief コールスタック基底
		*/
		class WalkerBase
		{
		public:
			/**
			 * @brief コールスタックを収集
			*/
			bool	Collect(const uint8 startDepth = 0);

			/**
			 * @brief シンボル情報を解決
			*/
			bool	Resolve();

			/**
			 * @brief クリア
			*/
			inline	void	Clear()noexcept {
				is_collected_ = false;
				is_resolved_ = false;
			}

			/**
			 * @brief	トレース出力
			*/
			void	Trace()const;

			/// @brief スタックトレースを文字列で取得
			nox::String	GetStackTraceString()const;

			/// @brief スタックトレースを文字列で取得
			std::span<char32>	GetStackTraceString(std::span<char32> dest_buffer)const;
			std::span<char16>	GetStackTraceU16String(std::span<char16> dest_buffer)const;

			[[nodiscard]] inline	constexpr	bool IsCollected()const noexcept { return is_collected_; }
			[[nodiscard]] inline	constexpr	bool IsResolved()const noexcept { return is_resolved_; }

			[[nodiscard]] inline	constexpr uint8 GetCollectLength()const noexcept { return collect_length_; }

			[[nodiscard]] inline	const StackFrame& GetStack(const uint8 index)const {
				NOX_ASSERT(index < collect_length_, nox::assertion::id::OutOfRange{}, u"コールスタックの取得に失敗");
				return stack_table_[index];
			}

			[[nodiscard]] inline	StackFrame& GetStack(const uint8 index){
				NOX_ASSERT(index < collect_length_, nox::assertion::id::OutOfRange{}, u"コールスタックの取得に失敗");
				return stack_table_[index];
			}

			inline void SetCollectLength(const uint8 length)
			{
				NOX_ASSERT(length <= stack_length_, nox::assertion::id::OutOfRange{}, u"コールスタックの取得に失敗");
				collect_length_ = length;
			}

			/// @brief 有効なスタックリストを取得
			[[nodiscard]] inline	std::span<const StackFrame> GetStackList()const noexcept { return std::span(stack_table_, stack_length_); }
		protected:
			constexpr WalkerBase()noexcept = delete;

			inline constexpr explicit WalkerBase(StackFrame* const stackTbl, const uint8 stackLength)noexcept :
				stack_table_(stackTbl),
				stack_length_(stackLength),
				collect_length_(0),
				is_collected_(false),
				is_resolved_(false)
			{}

			inline ~WalkerBase() = default;
		private:
			/// @brief スタックポインタ
			StackFrame* const stack_table_;

			/// @brief 最大スタック数
			const uint8 stack_length_;

			/// @brief コールスタック数
			uint8 collect_length_;

			/**
			 * @brief 有効なコールスタックを取得済みか
			*/
			bool	is_collected_;
			bool	is_resolved_;
		};
	}

	/// @brief コールスタック
	/// @tparam _STACK_DEPTH スタックの深さ
	template<uint8 _STACK_DEPTH = nox::stack_walker::DEFAULT_STACK_DEPTH> requires(_STACK_DEPTH <= MAX_STACK_DEPTH)
		class Walker : public detail::WalkerBase
	{
	public:
		inline constexpr Walker()noexcept :
			detail::WalkerBase(stack_table_.data(), static_cast<uint8>(stack_table_.size())) {}
		inline constexpr ~Walker() = default;

	private:
		inline constexpr Walker(const Walker&)noexcept = delete;
		inline constexpr Walker(const Walker&&)noexcept = delete;

	private:
		/// @brief スタック配列
		std::array<StackFrame, _STACK_DEPTH> stack_table_;
	};

	//template<uint8 _STACK_DEPTH = nox::stack_walker::DEFAULT_STACK_DEPTH> requires(_STACK_DEPTH <= MAX_STACK_DEPTH)
	//	class SlimWalker : public detail::WalkerBase
	//{
	//public:
	//	inline constexpr SlimWalker()noexcept :
	//		detail::WalkerBase(stack_table_.data(), static_cast<uint8>(stack_table_.size())) {}
	//	inline constexpr ~SlimWalker() = default;
	//	inline constexpr Walker(const Walker&)noexcept = delete;
	//	inline constexpr Walker(const Walker&&)noexcept = delete;
	//private:
	//	/// @brief スタック配列
	//	std::array<SlimStackFrame, _STACK_DEPTH> stack_table_;
	//};

	/// @brief stack_walker初期化
	void Initialize();

	/// @brief stack_walker終了処理
	void Finalize();

	/// @brief スタックトレース出力
	/// @param address_list アドレスリスト
	void Trace(std::span<const size_t> address_list);
}