//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	remote_entity.h
///	@brief	remote_entity
#pragma once

#if NOX_DEVELOP
#include	"../object.h"

namespace nox::dev::editor_ipc
{
	class SocketStreamWriter;
	class SocketStreamReader;

	class EditorIpcEntity : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::EditorIpcEntity, nox::Object);
	public:
		inline constexpr EditorIpcEntity() noexcept : id_(0) {}
		inline constexpr ~EditorIpcEntity() noexcept override {}

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
		struct ISendFromRuntimeTag { };
	}
}
#endif