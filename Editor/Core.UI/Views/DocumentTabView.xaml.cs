using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Core.UI.Views
{
	/// <summary>
	/// DocumentTabView.xaml の相互作用ロジック
	/// </summary>
	public partial class DocumentTabView : UserControl
	{
		private bool _initialized;

		public DocumentTabView()
		{
			InitializeComponent();
			DataContext = new ViewModels.DocumentTabViewModel();
		}

		private void TabControl_OnSelectionChanged(object sender, SelectionChangedEventArgs e)
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

		private void TabItem_OnPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			if (sender is not TabItem tabItem)
			{
				return;
			}

			// すでに選択されているタブを再クリックしたら閉じる
			if (tabItem.IsSelected && tabItem.DataContext is ViewModels.DocumentTabViewModel.DocumentTabItem { HasChildren: false })
			{
				if (ItemsControl.ItemsControlFromItemContainer(tabItem) is TabControl tabControl)
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
