//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	remote_entity.h
///	@brief	remote_entity
#pragma once

#if NOX_DEVELOP
#include	"../object.h"

namespace nox::dev::editor_remote
{
	class SocketStreamWriter;
	class SocketStreamReader;

	class EditorRemoteEntity : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::EditorRemoteEntity, nox::Object);
	public:
		inline constexpr EditorRemoteEntity() noexcept : id_(0) {}
		inline constexpr ~EditorRemoteEntity() override {}

		void Serialize(nox::uint32 id, SocketStreamWriter& writer);
		void Deserialize(SocketStreamReader& reader);

		inline constexpr nox::uint32 GetId()const noexcept { return id_; }
	protected:
		virtual void OnSerialize(SocketStreamWriter&) {}
		virtual void OnDeserialize(SocketStreamReader&) {}

	private:
		nox::uint32 id_;
	};

	namespace detail
	{
		/// @brief runtimeから送信されることを示すタグ
		struct ISendFromRuntimeTag { };
	}
}
#endif // NOX_DEVELOP