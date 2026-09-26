// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	type_name.h
///	@brief	type_name
#pragma once

#include	<type_traits>
#include	<string_view>
#include	<array>

#include	"../basic_definition.h"
#include	"../basic_type.h"
#include	"../crc32.h"
#include	"type_traits.h"
#include	"../preprocessor/cat.h"

namespace nox::util
{
	namespace detail
	{
		/// @brief コンパイラが綴った関数シグネチャから、テンプレート引数の部分だけを切り出す。
		/// @details 前置きの綴りはツールセットとそのバージョンで変わるため、固定長には依存せず
		///          目印の検索で位置を決める。clang は `... RawTypeName() [T = 型名]`、
		///          MSVC は `... nox::util::detail::RawTypeName<型名>(void) noexcept` を出す。
		///          どちらも型名の後ろに `]` / `>` が現れないので、末尾は逆方向の検索で確定できる。
		template<class T>
		[[nodiscard]] consteval std::string_view RawTypeName()noexcept
		{
#if defined(__clang__)
			constexpr std::string_view signature = __PRETTY_FUNCTION__;
			constexpr std::string_view marker = "[T = ";
			constexpr char terminator = ']';
#else
			constexpr std::string_view signature = __FUNCSIG__;
			constexpr std::string_view marker = "RawTypeName<";
			constexpr char terminator = '>';
#endif
			constexpr size_t markerPosition = signature.find(marker);
			constexpr size_t end = signature.rfind(terminator);
			static_assert(
				markerPosition != std::string_view::npos &&
				end != std::string_view::npos &&
				markerPosition + marker.size() <= end,
				"コンパイラが出す関数シグネチャの綴りが想定と違う (nox::util::detail::RawTypeName の目印を見直すこと)");

			return signature.substr(markerPosition + marker.size(), end - (markerPosition + marker.size()));
		}

		[[nodiscard]] inline constexpr bool IsTypeNameIdentifierChar(const char c)noexcept
		{
			return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
		}

		[[nodiscard]] inline constexpr bool IsTypeNameSpace(const char c)noexcept
		{
			return c == ' ' || c == '\t';
		}

		/// @brief rawのindex位置が token そのもの(識別子の途中ではない)で始まるか。
		[[nodiscard]] inline constexpr bool StartsTypeNameToken(
			const std::string_view raw, const size_t index, const std::string_view token)noexcept
		{
			return raw.compare(index, token.size(), token) == 0 &&
				(index + token.size() >= raw.size() ||
					nox::util::detail::IsTypeNameIdentifierChar(raw[index + token.size()]) == false);
		}

		/// @brief 正規化した型名を out へ書き出し、その長さを返す。out が nullptr なら長さだけ数える。
		/// @details ツールセット間で綴りが割れる要素を潰す。
		///          1. class / struct / enum / union と呼び出し規約を落とす (MSVCのみが綴る)
		///          2. MSVC固有の __int64 を long long へ読み替える
		///          3. 空白は「識別子同士の区切り」以外を全て落とす
		///             (`Pair<int, float>` と `Pair<int,float>`、`T *` と `T*`、`> >` と `>>` を揃える)
		[[nodiscard]] inline constexpr size_t WriteNormalizedTypeName(const std::string_view raw, char* const out)noexcept
		{
			//	MSVCだけが綴るためツールセット間の差になるトークン。取り除いて正規化する。
			//	class-key は clang が出さない。呼び出し規約と __ptr64 は関数型・ポインタ型に現れる。
			//	名前空間スコープの変数はリフレクション生成の対象になり、配列型
			//	(volatile const std::string_view[N]) の Type 実体化が通らないため関数内に置く。
			constexpr std::string_view k_dropped_tokens[] = {
				"class", "struct", "enum", "union",
				"__cdecl", "__stdcall", "__fastcall", "__vectorcall", "__thiscall", "__ptr64",
			};
			constexpr std::string_view k_msvc_int64 = "__int64";
			constexpr std::string_view k_int64 = "long long";

			size_t length = 0u;
			char previous = '\0';
			size_t index = 0u;
			while (index < raw.size())
			{
				const char c = raw[index];
				if (nox::util::detail::IsTypeNameSpace(c))
				{
					size_t next = index;
					while (next < raw.size() && nox::util::detail::IsTypeNameSpace(raw[next]))
					{
						++next;
					}

					//	識別子と識別子の間だけ、区切りの空白を1つ残す
					if (length != 0u && next < raw.size() &&
						nox::util::detail::IsTypeNameIdentifierChar(previous) &&
						nox::util::detail::IsTypeNameIdentifierChar(raw[next]))
					{
						if (out != nullptr)
						{
							out[length] = ' ';
						}
						previous = ' ';
						++length;
					}
					index = next;
					continue;
				}

				//	トークンの先頭でだけ、ツールセット固有の綴りを落とす / 読み替える
				if (index == 0u || nox::util::detail::IsTypeNameIdentifierChar(raw[index - 1u]) == false)
				{
					bool dropped = false;
					for (const std::string_view token : k_dropped_tokens)
					{
						if (nox::util::detail::StartsTypeNameToken(raw, index, token))
						{
							index += token.size();
							dropped = true;
							break;
						}
					}
					if (dropped)
					{
						continue;
					}

					if (nox::util::detail::StartsTypeNameToken(raw, index, k_msvc_int64))
					{
						for (const char replacement : k_int64)
						{
							if (out != nullptr)
							{
								out[length] = replacement;
							}
							previous = replacement;
							++length;
						}
						index += k_msvc_int64.size();
						continue;
					}
				}

				if (out != nullptr)
				{
					out[length] = c;
				}
				previous = c;
				++length;
				++index;
			}

			return length;
		}

		/// @brief 正規化済みの型名の実体。
		/// @details 定数初期化された静的記憶域なので、ヒープも動的初期化も使わない。
		///          正規化は元のシグネチャ文字列の部分列にならないため、実体を1つ持つ必要がある。
		template<class T>
		struct TypeNameHolder final
		{
			static constexpr size_t k_length = nox::util::detail::WriteNormalizedTypeName(
				nox::util::detail::RawTypeName<T>(), nullptr);

			static constexpr std::array<char, k_length + 1u> k_value = []()constexpr noexcept
				{
					std::array<char, k_length + 1u> buffer{};
					const size_t written = nox::util::detail::WriteNormalizedTypeName(
						nox::util::detail::RawTypeName<T>(), buffer.data());
					static_cast<void>(written);
					return buffer;
				}();
		};
	}

	/**
	 * @brief 型名を取得
	 * @details MSVC / clang-cl で同じ綴りになるよう正規化してある (class-key・空白・呼び出し規約)。
	 *          既定テンプレート引数の展開だけはツールセットの出力そのものが違うため揃わない
	 *          (例: std::vector<T> を MSVC は std::vector<T,std::allocator<T>> と綴る)。
	 *          用途は表示・ログ・アサートメッセージで、リテラルとの比較や永続化には使っていない。
	*/
	template <class T>// requires(!std::is_const_v<T> && !std::is_volatile_v<T>)
	[[nodiscard]] constexpr std::string_view GetTypeName(void)noexcept
	{
		return std::string_view(
			nox::util::detail::TypeNameHolder<T>::k_value.data(),
			nox::util::detail::TypeNameHolder<T>::k_length);
	}

	/// @brief 型IDを取得する
	/// @tparam T 型
	/// @return ID(4byte)
	/// @details crc32で余分な計算をしないよう、シグネチャ全体ではなく正規化済みの型名を渡す。
	///          正規化してあるのでMSVCとclang-clで同じ値になる。
	template<class T>
	inline constexpr uint32 GetUniqueTypeID()noexcept
	{
		return util::Crc32(nox::util::GetTypeName<T>());
	}

//	namespace detail
//	{
//	}
//
//	/// @brief 関数ポインタからIDを取得する
//	/// @tparam obj 関数ポインタ
//	/// @return ID
//	template<auto obj> requires(IsEveryFunctionV<decltype(obj)> || IsLambdaValue<decltype(obj)>)
//	inline constexpr u32 GetFunctionID()noexcept
//	{
//#if defined(__clang__)
//		return util::crc32(__PRETTY_FUNCTION__);
//#else
//		return util::crc32(NOX_DETAIL_TO_U8STRING(__FUNCSIG__));
//#endif // defined(__clang__)
//	}
}