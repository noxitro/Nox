#pragma once
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#if	NOX_DEVELOP
#include	"../editor_remote_query.h"
#include	"../editor_remote_response.h"

namespace nox::dev::editor_remote
{
	/// @brief リソースコンバートリクエスト
	class ResourceConvertQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::ResourceConvertQuery, nox::dev::editor_remote::Query);
	public:
		ResourceConvertQuery(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;

		inline std::u8string_view GetNativePath()const noexcept
		{
			return native_path_;
		}

		inline void SetNativePath(std::u8string_view value)
		{
			native_path_ = value;
		}

	private:
		nox::U8FixedString<256> native_path_ {};
	};

	/// @brief MainSceneViewを取得する
	class GetMainSceneView final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::GetMainSceneView, nox::dev::editor_remote::Query);
	public:
		GetMainSceneView()noexcept{}
		inline constexpr void OnSerialize(nox::dev::editor_remote::SocketStreamWriter&)override {}
		inline constexpr void OnDeserialize(nox::dev::editor_remote::SocketStreamReader&)override {}
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;
	};

	/// @brief SceneView情報
	class SceneViewInfo final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::SceneViewInfo, nox::dev::editor_remote::Response);
	public:
		SceneViewInfo(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline nox::int64 GetMainWindowHandle()const noexcept
		{
			return main_window_handle_;
		}

		inline void SetMainWindowHandle(nox::int64 value)
		{
			main_window_handle_ = value;
		}

	private:
		nox::int64 main_window_handle_ {};
	};

	/// @brief nox::Objectの同期Query
	class SyncQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::SyncQuery, nox::dev::editor_remote::Query);
	public:
		SyncQuery(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline std::u8string_view GetFqn()const noexcept
		{
			return fqn_;
		}

		inline void SetFqn(std::u8string_view value)
		{
			fqn_ = value;
		}

		inline const std::array<nox::uint8, 2048>& GetPropertyByteBuffer()const noexcept
		{
			return property_byte_buffer_;
		}

		inline void SetPropertyByteBuffer(const std::array<nox::uint8, 2048>& value)
		{
			property_byte_buffer_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		nox::U8FixedString<512> fqn_ {};
		std::array<nox::uint8, 2048> property_byte_buffer_ {};
	};

	/// @brief nox::Objectの同期結果
	class SyncResponse final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::SyncResponse, nox::dev::editor_remote::Response);
	public:
		SyncResponse(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline bool GetApplied()const noexcept
		{
			return applied_;
		}

		inline void SetApplied(bool value)
		{
			applied_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		bool applied_ {};
	};

	/// @brief Hierarchy EntityNode追加Query
	class AddEntityNodeQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::AddEntityNodeQuery, nox::dev::editor_remote::Query);
	public:
		AddEntityNodeQuery(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline nox::int64 GetParentRemoteInstanceId()const noexcept
		{
			return parent_remote_instance_id_;
		}

		inline void SetParentRemoteInstanceId(nox::int64 value)
		{
			parent_remote_instance_id_ = value;
		}

		inline std::u8string_view GetName()const noexcept
		{
			return name_;
		}

		inline void SetName(std::u8string_view value)
		{
			name_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		nox::int64 parent_remote_instance_id_ {};
		nox::U8FixedString<256> name_ {};
	};

	/// @brief Hierarchy EntityNode追加Response
	class AddEntityNodeResponse final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::AddEntityNodeResponse, nox::dev::editor_remote::Response);
	public:
		AddEntityNodeResponse(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline bool GetCreated()const noexcept
		{
			return created_;
		}

		inline void SetCreated(bool value)
		{
			created_ = value;
		}

		inline bool GetAttached()const noexcept
		{
			return attached_;
		}

		inline void SetAttached(bool value)
		{
			attached_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		bool created_ {};
		bool attached_ {};
	};

	/// @brief Hierarchy Component追加Query
	class AddComponentQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::AddComponentQuery, nox::dev::editor_remote::Query);
	public:
		AddComponentQuery(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline nox::int64 GetEntityNodeRemoteInstanceId()const noexcept
		{
			return entity_node_remote_instance_id_;
		}

		inline void SetEntityNodeRemoteInstanceId(nox::int64 value)
		{
			entity_node_remote_instance_id_ = value;
		}

		inline std::u8string_view GetComponentTypeFqn()const noexcept
		{
			return component_type_fqn_;
		}

		inline void SetComponentTypeFqn(std::u8string_view value)
		{
			component_type_fqn_ = value;
		}

		inline const std::array<nox::uint8, 2048>& GetPropertyByteBuffer()const noexcept
		{
			return property_byte_buffer_;
		}

		inline void SetPropertyByteBuffer(const std::array<nox::uint8, 2048>& value)
		{
			property_byte_buffer_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		nox::int64 entity_node_remote_instance_id_ {};
		nox::U8FixedString<512> component_type_fqn_ {};
		std::array<nox::uint8, 2048> property_byte_buffer_ {};
	};

	/// @brief Hierarchy Component追加Response
	class AddComponentResponse final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::AddComponentResponse, nox::dev::editor_remote::Response);
	public:
		AddComponentResponse(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline bool GetCreated()const noexcept
		{
			return created_;
		}

		inline void SetCreated(bool value)
		{
			created_ = value;
		}

		inline bool GetAdded()const noexcept
		{
			return added_;
		}

		inline void SetAdded(bool value)
		{
			added_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		bool created_ {};
		bool added_ {};
	};

	/// @brief Hierarchy EntityNode破棄Query
	class DestroyEntityNodeQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::DestroyEntityNodeQuery, nox::dev::editor_remote::Query);
	public:
		DestroyEntityNodeQuery(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
	};

	/// @brief Inspector Auto-Sync Query
	class AutoSyncQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::AutoSyncQuery, nox::dev::editor_remote::Query);
	public:
		AutoSyncQuery(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
	};

	/// @brief Inspector Auto-Sync Response
	class AutoSyncResponse final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::AutoSyncResponse, nox::dev::editor_remote::Response);
	public:
		AutoSyncResponse(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline bool GetExists()const noexcept
		{
			return exists_;
		}

		inline void SetExists(bool value)
		{
			exists_ = value;
		}

		inline const std::array<nox::uint8, 2048>& GetPropertyByteBuffer()const noexcept
		{
			return property_byte_buffer_;
		}

		inline void SetPropertyByteBuffer(const std::array<nox::uint8, 2048>& value)
		{
			property_byte_buffer_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		bool exists_ {};
		std::array<nox::uint8, 2048> property_byte_buffer_ {};
	};

	/// @brief Inspector Action関数実行Query
	class InvokeRuntimeActionQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::InvokeRuntimeActionQuery, nox::dev::editor_remote::Query);
	public:
		InvokeRuntimeActionQuery(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline std::u8string_view GetFunctionFullName()const noexcept
		{
			return function_full_name_;
		}

		inline void SetFunctionFullName(std::u8string_view value)
		{
			function_full_name_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		nox::U8FixedString<512> function_full_name_ {};
	};

	/// @brief Inspector Action関数実行Response
	class InvokeRuntimeActionResponse final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::InvokeRuntimeActionResponse, nox::dev::editor_remote::Response);
	public:
		InvokeRuntimeActionResponse(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

		inline bool GetInvoked()const noexcept
		{
			return invoked_;
		}

		inline void SetInvoked(bool value)
		{
			invoked_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
		bool invoked_ {};
	};

	/// @brief Runtime依存グラフ取得Query
	class GetRuntimeDependencyGraphQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::GetRuntimeDependencyGraphQuery, nox::dev::editor_remote::Query);
	public:
		GetRuntimeDependencyGraphQuery()noexcept{}
		inline constexpr void OnSerialize(nox::dev::editor_remote::SocketStreamWriter&)override {}
		inline constexpr void OnDeserialize(nox::dev::editor_remote::SocketStreamReader&)override {}
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;
	};

	/// @brief Runtime依存グラフ取得Response
	class RuntimeDependencyGraphResponse final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::RuntimeDependencyGraphResponse, nox::dev::editor_remote::Response);
	public:
		RuntimeDependencyGraphResponse(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline std::u8string_view GetGraphText()const noexcept
		{
			return graph_text_;
		}

		inline void SetGraphText(std::u8string_view value)
		{
			graph_text_ = value;
		}

	private:
		nox::U8FixedString<3072> graph_text_ {};
	};

	/// @brief RemoteInstance管理状態取得Query
	class GetRemoteInstanceSnapshotQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::GetRemoteInstanceSnapshotQuery, nox::dev::editor_remote::Query);
	public:
		GetRemoteInstanceSnapshotQuery()noexcept{}
		inline constexpr void OnSerialize(nox::dev::editor_remote::SocketStreamWriter&)override {}
		inline constexpr void OnDeserialize(nox::dev::editor_remote::SocketStreamReader&)override {}
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;
	};

	/// @brief RemoteInstance管理状態取得Response
	class RemoteInstanceSnapshotResponse final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::RemoteInstanceSnapshotResponse, nox::dev::editor_remote::Response);
	public:
		RemoteInstanceSnapshotResponse(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline std::u8string_view GetSnapshotText()const noexcept
		{
			return snapshot_text_;
		}

		inline void SetSnapshotText(std::u8string_view value)
		{
			snapshot_text_ = value;
		}

	private:
		nox::U8FixedString<3072> snapshot_text_ {};
	};

	/// @brief MemoryProfilerスナップショット取得Query
	class GetMemoryProfilerSnapshotQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::GetMemoryProfilerSnapshotQuery, nox::dev::editor_remote::Query);
	public:
		GetMemoryProfilerSnapshotQuery()noexcept{}
		inline constexpr void OnSerialize(nox::dev::editor_remote::SocketStreamWriter&)override {}
		inline constexpr void OnDeserialize(nox::dev::editor_remote::SocketStreamReader&)override {}
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;
	};

	/// @brief MemoryProfilerスナップショット取得Response
	class MemoryProfilerSnapshotResponse final : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::MemoryProfilerSnapshotResponse, nox::dev::editor_remote::Response);
	public:
		MemoryProfilerSnapshotResponse(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;

		inline std::u8string_view GetSnapshotText()const noexcept
		{
			return snapshot_text_;
		}

		inline void SetSnapshotText(std::u8string_view value)
		{
			snapshot_text_ = value;
		}

	private:
		nox::U8FixedString<3072> snapshot_text_ {};
	};

	/// @brief Runtime側RemoteObject破棄通知Query
	class RuntimeObjectDestroyedQuery final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::RuntimeObjectDestroyedQuery, nox::dev::editor_remote::Query);
	public:
		RuntimeObjectDestroyedQuery(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		inline constexpr nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8>)const override { return nullptr; }

		inline nox::int64 GetRemoteInstanceId()const noexcept
		{
			return remote_instance_id_;
		}

		inline void SetRemoteInstanceId(nox::int64 value)
		{
			remote_instance_id_ = value;
		}

	private:
		nox::int64 remote_instance_id_ {};
	};

}
#endif	//	NOX_DEVELOP
