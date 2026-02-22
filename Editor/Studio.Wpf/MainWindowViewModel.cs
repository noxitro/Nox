using System;
using System.Collections.Generic;
using System.Text;

namespace Studio.Wpf.ViewModels
{
	public class MainWindowViewModel : NoxUI.ViewModelBase
	{
		public MainWindowViewModel()
		{
		}

		#region 非公開フィールド
		//	dockingmanager
		AvalonDock.DockingManager? _DockingManager = null;

		private Core.UI.ViewModels.ToolViewModel[] _ToolViewModelList = [];
		private Core.UI.ViewModels.AssetViewModel[] _AssetViewModelList = [];
		#endregion

		#region 公開プロパティ
		public IReadOnlyList<Core.UI.ViewModels.ToolViewModel> ToolViewModelList => _ToolViewModelList;
		public IReadOnlyList<Core.UI.ViewModels.AssetViewModel> AssetViewModelList => _AssetViewModelList;
		#endregion

		#region 公開メソッド
		public void Loaded(AvalonDock.DockingManager dockingManager)
		{
			_DockingManager = dockingManager;

			//_ToolViewModelList = ;
		}

		public void Unloaded(AvalonDock.DockingManager dockingManager)
		{
			if (_DockingManager == dockingManager)
			{
				_DockingManager = null;
			}
		}
		#endregion
	}
}
