//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	stack_trace.h
///	@brief	stack_trace
#pragma once
#include	"basic_type.h"

#include	"nox_string.h"
#include	"nox_string_view.h"

#include	"assertion.h"

namespace nox::stack_walker
{
	/// @brief 最大スタック数
	constexpr uint8 MAX_STACK_DEPTH = 32U;

	/// @brief スタック情報
	class Stack
	{
	public:
		inline constexpr Stack()noexcept :
			address_(0),
			module_name_{ u'\000' },
			file_name_{ u'\000' },
			symbol_name_{ u'\000' },
			line_(0),
			is_resolver_(false)
		{
		}

		inline consteval Stack(const Stack&)noexcept = delete;
		inline consteval Stack(Stack&&)noexcept = delete;

		inline constexpr ~Stack() = default;

		inline	void	SetAddress(const std::size_t address)noexcept { address_ = address; }
		inline	void	SetResolved(bool isResolver)noexcept { is_resolver_ = isResolver; }
		void	SetModuleName(StringView name);
		void	SetFileName(StringView name);
		void	SetSymbolName(StringView name);
		inline	void	SetLine(const uint32 line)noexcept { line_ = line; }

		[[nodiscard]]	inline	constexpr	std::size_t	GetAddress()const noexcept { return address_; }
		[[nodiscard]]	inline	constexpr	StringView GetModuleName()const noexcept { return module_name_.data(); }
		[[nodiscard]]	inline	constexpr	StringView GetSymbolName()const noexcept { return symbol_name_.data(); }
		[[nodiscard]]	inline	constexpr	StringView GetFileName()const noexcept { return file_name_.data(); }
		[[nodiscard]]	inline	constexpr	uint32	GetLine()const noexcept { return line_; }

		/// @brief リゾルブ済みか
		/// @return リゾルブ済みか
		[[nodiscard]] inline	constexpr	bool	IsResolved()const noexcept { return is_resolver_; }

		/// @brief 無効なアドレスかどうかを取得
		/// @return 無効なアドレスかどうか
		[[nodiscard]] inline	constexpr	bool	IsInvalidAddress()const noexcept { return address_ == 0U; }
	private:
		/// @brief アドレス
		std::size_t address_;

		/// @brief モジュール名
		std::array<char32, 512> module_name_;

		/// @brief ファイル名
		std::array<char32, 1024> file_name_;

		/// @brief シンボル名
		std::array<char32, 512> symbol_name_;

		/// @brief 行番号
		uint32 line_;

		/// @brief 解決済み
		bool	is_resolver_;
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

			[[nodiscard]] inline	const Stack& GetStack(const uint8 index)const {
				NOX_ASSERT(index < collect_length_, nox::assertion::RuntimeAssertErrorType::OutOfRange, U"コールスタックの取得に失敗　範囲外アクセス");
				return stack_table_[index];
			}

			[[nodiscard]] inline	std::span<Stack* const> GetStackList()const noexcept { return std::span(&stack_table_, stack_length_); }
		protected:
			constexpr WalkerBase()noexcept = delete;

			inline constexpr explicit WalkerBase(Stack* const stackTbl, const uint8 stackLength)noexcept :
				stack_table_(stackTbl),
				stack_length_(stackLength),
				collect_length_(0),
				is_collected_(false),
				is_resolved_(false)
			{}

			inline ~WalkerBase() = default;
		private:
			/**
			 * @brief スタックテーブル
			*/
			Stack* const stack_table_;

			/**
			 * @brief 最大スタック数
			*/
			const uint8 stack_length_;

			/**
			 * @brief コールスタック数
			*/
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
	template<uint8 _STACK_DEPTH> requires(_STACK_DEPTH <= MAX_STACK_DEPTH)
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
		std::array<Stack, _STACK_DEPTH> stack_table_;
	};

	/**
	 * @brief 初期化
	*/
	void Initialize();

	/**
	 * @brief 終了処理
	*/
	void Finalize();
}