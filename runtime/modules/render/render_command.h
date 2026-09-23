//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_command.h
///	@brief	render_command
#pragma once
#include	"render_command_type.h"

namespace nox::render::commands
{
	struct CommandBase
	{
		union
		{
			//!<@brief	優先度が値が小さいほど先に処理されます
			struct Order
			{
				nox::uint8 priority;	//!<@brief	優先度
				nox::uint8 segment;	//!<@brief	定義優先
				nox::uint8 layer;		//!<@brief	レイヤー
				nox::uint8 contextId;		//!<@brief	並列処理用(contextID)

				inline constexpr Order()noexcept :
					priority(0)
					, segment(0)
					, layer(0)
					, contextId(0) {
				}
			};
			static_assert(sizeof(Order) == sizeof(nox::uint32));
			Order order;			//!<@brief	オーダー
			nox::uint32 command_order;		//!<@brief	ソートオーダー
		};

		const CommandType command_type;
		const nox::uint32 command_size;

	protected:
		inline constexpr CommandBase(CommandType command_type, const nox::uint32 size)noexcept:
			order(),
			command_type(command_type),
			command_size(size)
		{}
		~CommandBase() {}
	};

	namespace detail
	{
		template<class T, CommandType _CommandType>
		struct CommandBaseInternal : CommandBase
		{
			static constexpr CommandType Type = _CommandType;
		public:
			inline constexpr CommandBaseInternal()noexcept :
				CommandBase(_CommandType, sizeof(T))
			{
			}
			~CommandBaseInternal()noexcept = default;
		};
	}

	struct BeginMarker : nox::render::commands::detail::CommandBaseInternal<BeginMarker, CommandType::BeginMarker>
	{
		inline constexpr explicit	BeginMarker(const std::u8string_view name) noexcept :
			name_(name)
		{
		}

		const std::u8string_view name_;
	};

	struct EndMarker : nox::render::commands::detail::CommandBaseInternal<EndMarker, CommandType::EndMarker>
	{
	};
}