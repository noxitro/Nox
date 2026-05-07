
using System.Runtime.CompilerServices;

namespace Core.UI
{
	public abstract class EntryBase
	{
		public virtual void RegisterTypes(Prism.Ioc.IContainerRegistry containerRegistry)
		{

		}
	}

	file class Entry : EntryBase, IDisposable
	{
		public Entry()
		{
			Core.UI.MessageServiceProvider.Register(new Core.UI.WpfMessageService());
		}

		public override void RegisterTypes(IContainerRegistry containerRegistry)
		{
			containerRegistry.RegisterInstance(Core.StudioManager.Instance.Workspace.LogService);
			containerRegistry.Register<Core.UI.ViewModels.RuntimeControlViewModel>();
			containerRegistry.Register<Core.UI.ViewModels.MessageBoxWindowViewModel>();
			containerRegistry.Register<Core.UI.ViewModels.RuntimeViewModel>();
			containerRegistry.Register<Core.UI.ViewModels.HierarchyViewModel>();
			containerRegistry.Register<Core.UI.ViewModels.AssetBrowserViewModel>();
			containerRegistry.Register<Core.UI.ViewModels.InspectorViewModel>();
			containerRegistry.Register<Core.UI.ViewModels.LogViewModel>();
			containerRegistry.Register<Core.UI.ViewModels.ProjectSettingsViewModel>();
		}

		void IDisposable.Dispose()
		{

		}
	}
}
