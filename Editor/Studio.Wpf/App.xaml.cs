using System.Configuration;
using System.Data;
using System.Windows;

namespace Studio.Wpf
{
	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Prism.Unity.PrismApplication
	{
		private List<Core.UI.EntryBase> UIEntryList = new();

		public App()
		{
			Core.EntryManager.CreateInstance();

			System.Type entryType = typeof(Core.EntryBase);
			System.Type uiEntryType = typeof(Core.UI.EntryBase);

			foreach (System.Type type in Core.TypeDB.AllTypeList)
			{
				if (entryType != type && entryType.IsAssignableFrom(type))
				{
					Core.EntryBase entry = (Core.EntryBase)System.Activator.CreateInstance(type)!;
				}

				if (uiEntryType != type && uiEntryType.IsAssignableFrom(type))
				{
					Core.UI.EntryBase entry = (Core.UI.EntryBase)System.Activator.CreateInstance(type)!;
					UIEntryList.Add(entry);
				}
			}
		}

		protected override void OnStartup(StartupEventArgs e)
		{
			base.OnStartup(e);

			System.Threading.Tasks.TaskScheduler.UnobservedTaskException += OnUnobservedTaskException;

			Core.EntryManager.Instance.InvokeStart();
		}

		protected override void OnExit(ExitEventArgs e)
		{
			base.OnExit(e);

			Core.EntryManager.Instance.InvokeFinalize();
			Core.EntryManager.DeleteInstance();
		}

		[System.Runtime.Versioning.SupportedOSPlatform("windows10.0")]
		protected override Window CreateShell()
		{
			return Container.Resolve<MainWindow>();
		}

		[System.Runtime.Versioning.SupportedOSPlatform("windows10.0")]
		protected override void ConfigureViewModelLocator()
		{
			base.ConfigureViewModelLocator();

		}

		[System.Runtime.Versioning.SupportedOSPlatform("windows10.0")]
		protected override void RegisterTypes(IContainerRegistry containerRegistry)
		{
			foreach(var entry in UIEntryList)
			{
				entry.RegisterTypes(containerRegistry);
			}
		}

		private void OnUnobservedTaskException(object? sender, System.Threading.Tasks.UnobservedTaskExceptionEventArgs e)
		{
			
			var ex = e.Exception; // AggregateException

			// ログ
			Nox.LogTrace.ErrorLine<Core.LogId.Runtime>(
				"Unobserved Task exception: {0}", ex);

			// ここで自作 MessageBox を表示したり、Runtime へ流したりできる
			var vm = new Core.UI.ViewModels.MessageBoxViewModel(
				ex.InnerException?.Message ?? ex.Message,
				type: Core.UI.ViewModels.MessageBoxType.Error);

			var win = new Core.UI.Views.MessageBoxWindow
			{
				Owner = Current.MainWindow,
				DataContext = vm
			};
			win.ShowDialog();

			// 例外を「処理済み」にしてプロセス終了を防ぐ
			e.SetObserved();
		}
	}

}
