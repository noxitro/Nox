//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	application.h
///	@brief	application
#pragma once
#include	"attribute_common.h"
#include	"attribute_dev_common.h"

#include	"engine_system.h"

namespace nox
{
	class EngineModule;
	class SceneView;

	/// @brief Coreの管理クラス
	class 
		NOX_ATTR_TYPE(::nox::attr::dev::Description(u8"Application"), nox::attr::dev::DisplayName(u8"アプリケーション"))
		Application : public nox::EngineSystem
	{
		NOX_DECLARE_OBJECT(Application, nox::EngineSystem);
	private:
		/// @brief 実行ノード
		struct ExecuteNode
		{
			std::reference_wrapper<nox::EngineSystem> instance;
			std::reference_wrapper<const nox::EngineSystem::SystemPhase> phase;
			nox::uint32 layer_index;	///< 小さいほど先に実行。同一レイヤーは並列実行可能
		};
	public:
		Application()noexcept;
		~Application()override;
		
		void	Run();

		inline	constexpr nox::uint32 GetFrameCount()const noexcept { return frame_counter_; }
		inline	constexpr nox::uint16 GetTargetFrameRate()const noexcept { return target_frame_rate_; }
		inline	constexpr bool EnabledVSync()const noexcept { return enabled_vsync_; }
		void SetVSync(bool flag)noexcept;

		inline	constexpr bool IsKill()const noexcept { return kill_; }

		nox::EngineSystem* FindSystem(const nox::reflection::Type& type)const noexcept;

		template<std::derived_from<nox::EngineSystem> T>
		inline T* FindSystem()const noexcept
		{
			return static_cast<T*>(FindSystem(nox::reflection::Typeof<T>()));
		}

		nox::EngineSystem& GetSystem(const nox::reflection::Type& type)const;

		template<std::derived_from<nox::EngineSystem> T>
		inline T& GetSystem()const
		{
			return static_cast<T&>(GetSystem(nox::reflection::Typeof<T>()));
		}

		inline bool IsStudioMode()const noexcept { return studio_mode_; }
	private:
		void	Init();
		void	Update();
		void	Exit();

		void BuildExecuteNodeList(std::span<nox::EngineSystem*> system_list);
		void ExecutePhase(const nox::SystemPhaseType phase_type);
		void RegisterEngineSystem(nox::EngineSystem& engine_system);

#if !NOX_MASTER
		/// @brief 依存関係を含めた実行ノードリストを出力
		void TraceExecuteNodeList()const;
#endif // !NOX_MASTER

		std::span<const nox::EngineSystem::PhaseRegister> GetPhaseRegisterList()const noexcept override;
	private:
		bool kill_;
		nox::uint16 target_frame_rate_;
		bool enabled_vsync_;

		nox::StopWatch stop_watch_;
		nox::uint32 frame_counter_;
		nox::float_t elapsed_milli_seconds_;
		nox::float_t next_elapsed_milli_seconds_;

		nox::Vector<std::reference_wrapper<nox::EngineModule>> module_entry_list_;

		nox::UnorderedMap<const nox::reflection::Type*, nox::EngineSystem*> engine_system_map_;
		std::array<nox::Vector<ExecuteNode>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> system_phase_table_;

		/// @brief Studioから起動されたか
		const bool studio_mode_;
	};
}