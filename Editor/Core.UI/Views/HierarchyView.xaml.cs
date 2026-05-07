using System;
using System.Collections.Generic;

namespace Core.UI.Views
{
	/// <summary>
	/// HierarchyView.xaml の相互作用ロジック
	/// </summary>
	public partial class HierarchyView : System.Windows.Controls.UserControl
	{
		public HierarchyView()
		{
			InitializeComponent();
		}

		private void OnSelectedItemChanged(object sender, System.Windows.RoutedPropertyChangedEventArgs<object> e)
		{
			if (DataContext is Core.UI.ViewModels.HierarchyViewModel viewModel)
			{
				viewModel.SelectedNode = e.NewValue as Core.UI.ViewModels.HierarchyNodeViewModel;
			}
		}
	}
}
