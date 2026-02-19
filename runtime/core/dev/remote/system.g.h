#pragma once
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#if	NOX_DEVELOP
// forward declaration for runtime wrapper types
namespace nox { class SceneView; }
// end forward declaration

#include	"../editor_ipc_query.h"
#include	"../editor_ipc_response.h"

namespace nox::dev::editor_ipc
{
	/// @brief リソースコンバートリクエスト
	class ResourceConvertQuery final : public nox::dev::editor_ipc::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::ResourceConvertQuery, nox::dev::editor_ipc::Query);
	public:
		ResourceConvertQuery(){}
		void OnSerialize(nox::dev::editor_ipc::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_ipc::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_ipc::Response> Execute(std::span<nox::uint8> storage)const override;

		inline std::u8string_view GetNativePath()noexcept
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
	class GetMainSceneView final : public nox::dev::editor_ipc::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::GetMainSceneView, nox::dev::editor_ipc::Query);
	public:
		GetMainSceneView()noexcept{}
		inline constexpr void OnSerialize(nox::dev::editor_ipc::SocketStreamWriter&)override {}
		inline constexpr void OnDeserialize(nox::dev::editor_ipc::SocketStreamReader&)override {}
		nox::PlacementObject<nox::dev::editor_ipc::Response> Execute(std::span<nox::uint8> storage)const override;
	};

	/// @brief SceneView情報
	class SceneViewInfo final : public nox::dev::editor_ipc::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::SceneViewInfo, nox::dev::editor_ipc::Response);
	public:
		SceneViewInfo(){}
		void OnSerialize(nox::dev::editor_ipc::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_ipc::SocketStreamReader& reader)override;

		inline nox::int64 GetMainWindowHandle()noexcept
		{
			return main_window_handle_;
		}
		inline void SetMainWindowHandle(nox::int64 value)
		{
			main_window_handle_ = value;
		}
		inline const nox::IntrusivePtr<nox::SceneView>& GetSceneView()noexcept
		{
			return scene_view_;
		}
		inline void SetSceneView(nox::SceneView* value)
		{
			scene_view_ = value;
		}

	private:
		nox::int64 main_window_handle_ {};
		nox::IntrusivePtr<nox::SceneView> scene_view_ {};
	};

}
#endif	//	NOX_DEVELOP
