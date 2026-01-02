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
		protected override void OnStartup(StartupEventArgs e)
		{
			base.OnStartup(e);

			Core.Net.SocketScheduler.CreateInstance();
			Core.Net.RuntimeIpcClient.CreateInstance();
			Core.Runtime.CreateInstance();
		}

		protected override void OnExit(ExitEventArgs e)
		{
			base.OnExit(e);

			Core.Runtime.DeleteInstance();
			Core.Net.RuntimeIpcClient.DeleteInstance();
			Core.Net.SocketScheduler.DeleteInstance();
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
		}
	}

}
