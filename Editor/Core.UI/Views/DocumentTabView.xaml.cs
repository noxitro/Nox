using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.Views
{
	/// <summary>
	/// DocumentTabView.xaml の相互作用ロジック
	/// </summary>
	public partial class DocumentTabView : System.Windows.Controls.UserControl
	{
		private bool _initialized;

		public DocumentTabView()
		{
			InitializeComponent();
			DataContext = new ViewModels.DocumentTabViewModel();
		}

		private void TabControl_OnSelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
		{
			if (!_initialized)
			{
				_initialized = true;
				return; // 初期選択時のダイアログを抑止
			}

			if (e.AddedItems.Count > 0 && e.AddedItems[0] is ViewModels.DocumentTabViewModel.DocumentTabItem item)
			{
				if (DataContext is ViewModels.DocumentTabViewModel vm && item.Command != null)
				{
					if (item.Command.CanExecute())
					{
						item.Command.Execute();
					}
				}
			}
		}

		private void TabItem_OnPreviewMouseLeftButtonDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (sender is not System.Windows.Controls.TabItem tabItem)
			{
				return;
			}

			// すでに選択されているタブを再クリックしたら閉じる
			if (tabItem.IsSelected && tabItem.DataContext is ViewModels.DocumentTabViewModel.DocumentTabItem { HasChildren: false })
			{
				if (System.Windows.Controls.ItemsControl.ItemsControlFromItemContainer(tabItem) is System.Windows.Controls.TabControl tabControl)
				{
					tabControl.SelectedIndex = -1;
					e.Handled = true;
				}
				return;
			}

			if (tabItem.DataContext is ViewModels.DocumentTabViewModel.DocumentTabItem { HasChildren: true })
			{
				if (tabItem.ContextMenu != null)
				{
					tabItem.ContextMenu.PlacementTarget = tabItem;
					tabItem.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
					// toggle open/close
					tabItem.ContextMenu.IsOpen = !tabItem.ContextMenu.IsOpen;
					e.Handled = true;
				}
			}
		}
	}
}
