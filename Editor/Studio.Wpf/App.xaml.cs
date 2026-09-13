// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.Configuration;
using System.Data;
using System.Windows;
using Microsoft.Extensions.DependencyInjection;

namespace Studio.Wpf;

	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application
	{
		private List<Core.UI.EntryBase> UIEntryList = new();
		private ServiceProvider? _ServiceProvider;

		public App()
		{
			Core.EngineModule.CreateInstance();

			System.Type entryType = typeof(Core.EngineModule);
			System.Type uiEntryType = typeof(Core.UI.EntryBase);

			foreach (System.Type type in Core.TypeDB.AllTypeList)
			{
				if (entryType != type && entryType.IsAssignableFrom(type))
				{
					Core.EngineModule entry = (Core.EngineModule)System.Activator.CreateInstance(type)!;
				}

				if (uiEntryType != type && uiEntryType.IsAssignableFrom(type))
				{
					Core.UI.EntryBase entry = (Core.UI.EntryBase)System.Activator.CreateInstance(type)!;
					UIEntryList.Add(entry);
				}
			}
		}

    private static Dictionary<string, string> ParseArguments(ReadOnlySpan<string> args)
    {
        Dictionary<string, string> result = new(StringComparer.OrdinalIgnoreCase);

        foreach (string arg in args)
        {
            if (!arg.StartsWith("--", StringComparison.Ordinal))
            {
                continue;
            }

            int separatorIndex = arg.IndexOf('=');
            if (separatorIndex < 0)
            {
                result[arg[2..]] = "true";
                continue;
            }

            string key = arg[2..separatorIndex];
            string value = arg[(separatorIndex + 1)..].Trim('"');
            result[key] = value;
        }

        return result;
    }

    protected override void OnStartup(StartupEventArgs e)
		{
			var args = ParseArguments(e.Args);
			if (args.TryGetValue("ProjectPath", out string? projectPath))
			{
				Core.StudioManager.ConfigureProjectPath(projectPath);
			}
			else if (args.TryGetValue("EngineRootDir", out string? legacyProjectPath))
			{
				Core.StudioManager.ConfigureProjectPath(legacyProjectPath);
			}
			else
			{
				Core.StudioManager.ConfigureProjectPath(null);
			}

        System.Threading.Tasks.TaskScheduler.UnobservedTaskException += OnUnobservedTaskException;

			Core.EngineModule.Instance.InvokeStart();

			base.OnStartup(e);

			// DI コンテナを組み立ててからシェルを出す。順序は Prism 時代と同じで、
			// EngineModule の起動 (InvokeStart) が先、登録 (ConfigureServices) が後。
			ServiceCollection services = new();
			ConfigureServices(services);
			_ServiceProvider = services.BuildServiceProvider();
			NoxUI.ServiceLocator.Current = _ServiceProvider;

			MainWindow shell = _ServiceProvider.GetRequiredService<MainWindow>();
			MainWindow = shell;
			shell.Show();
		}

		protected override void OnExit(ExitEventArgs e)
		{
			Core.EngineModule.DeleteInstance();
			NoxUI.ServiceLocator.Current = null;
			_ServiceProvider?.Dispose();
			base.OnExit(e);
		}

		private void ConfigureServices(IServiceCollection services)
		{
			//	テーマサービス（シングルトン）
			services.AddSingleton<Studio.Wpf.Themes.IThemeService, Studio.Wpf.Themes.ThemeService>();
			services.AddTransient<Studio.Wpf.ViewModels.ThemeSettingsViewModel>();
			services.AddTransient<Studio.Wpf.ViewModels.MainWindowViewModel>();
			services.AddTransient<MainWindow>();

			foreach(var entry in UIEntryList)
			{
				entry.RegisterTypes(services);
			}
		}

	//	[System.Runtime.Versioning.SupportedOSPlatform("windows10.0")]
		private void OnUnobservedTaskException(object? sender, System.Threading.Tasks.UnobservedTaskExceptionEventArgs e)
		{
			var ex = e.Exception; // AggregateException

			// ログ
			Nox.LogTrace.ErrorLine<Core.LogId.Runtime>(
				"Unobserved Task exception: {0}", ex);

			// ここで自作 MessageBox を表示したり、Runtime へ流したりできる


			//var win = new Core.UI.Views.MessageBoxWindow(vm)
			//{
			//	Owner = Current.MainWindow,
			//};
			//win.ShowDialog();

			// 例外を「処理済み」にしてプロセス終了を防ぐ
			e.SetObserved();
		}
	}
