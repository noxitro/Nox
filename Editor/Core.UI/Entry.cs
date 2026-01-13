
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
			Core.MessageServiceProvider.Register(new Core.UI.WpfMessageService());
		}

		public override void RegisterTypes(IContainerRegistry containerRegistry)
		{
			containerRegistry.Register<Core.UI.ViewModels.RuntimeControlViewModel>();
			containerRegistry.Register<Core.UI.ViewModels.MessageBoxViewModel>();
		}

		void IDisposable.Dispose()
		{

		}
	}
}
