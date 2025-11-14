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

		void Serialize(SocketStreamWriter& writer);
		void Deserialize(SocketStreamReader& reader);

	protected:
		virtual void OnSerialize(SocketStreamWriter& writer) {}
		virtual void OnDeserialize(SocketStreamReader& reader) {}

	private:
		nox::uint32 id_;
	};
}
#endif