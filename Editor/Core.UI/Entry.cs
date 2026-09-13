// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.Runtime.CompilerServices;
using Microsoft.Extensions.DependencyInjection;

namespace Core.UI;

	public abstract class EntryBase
	{
		public virtual void RegisterTypes(IServiceCollection services)
		{

		}
	}

	file class Entry : EntryBase, IDisposable
	{
		public Entry()
		{
			Core.UI.MessageServiceProvider.Register(new Core.UI.WpfMessageService());
		}

		public override void RegisterTypes(IServiceCollection services)
		{
			// Prism の RegisterInstance と同じく、宣言型 (コンパイル時の型) で登録する
			RegisterInstance(services, Core.StudioManager.Instance.Workspace.LogService);
			services.AddTransient<Core.UI.ViewModels.RuntimeControlViewModel>();
			services.AddTransient<Core.UI.ViewModels.MessageBoxWindowViewModel>();
			services.AddTransient<Core.UI.ViewModels.RuntimeViewModel>();
			services.AddTransient<Core.UI.ViewModels.HierarchyViewModel>();
			services.AddTransient<Core.UI.ViewModels.AssetBrowserViewModel>();
			services.AddTransient<Core.UI.ViewModels.AssetPropertiesViewModel>();
			services.AddTransient<Core.UI.ViewModels.InspectorViewModel>();
			services.AddTransient<Core.UI.ViewModels.LogViewModel>();
			services.AddTransient<Core.UI.ViewModels.ProjectSettingsViewModel>();
			services.AddTransient<Core.UI.ViewModels.EngineSystemGraphViewModel>();
			services.AddTransient<Core.UI.ViewModels.RemoteInstanceManagerViewModel>();
			services.AddTransient<Core.UI.ViewModels.MemoryProfilerViewModel>();
		}

		void IDisposable.Dispose()
		{

		}

		private static void RegisterInstance<T>(IServiceCollection services, T instance) where T : class
		{
			services.AddSingleton<T>(instance);
		}
	}
