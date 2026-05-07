namespace Core.UI.Views
{
	/// <summary>
	/// AssetBrowserView.xaml の相互作用ロジック
	/// </summary>
	public partial class AssetBrowserView : System.Windows.Controls.UserControl
	{
		public AssetBrowserView()
		{
			InitializeComponent();
		}

		private void OnSelectedItemChanged(object sender, System.Windows.RoutedPropertyChangedEventArgs<object> e)
		{
			if (DataContext is Core.UI.ViewModels.AssetBrowserViewModel viewModel)
			{
				viewModel.SelectedTreeNode = e.NewValue as Core.UI.ViewModels.AssetTreeNodeViewModel;
			}
		}

		private void OnAssetDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (DataContext is Core.UI.ViewModels.AssetBrowserViewModel viewModel &&
				viewModel.SelectedAsset != null &&
				viewModel.OpenInExplorerCommand.CanExecute())
			{
				viewModel.OpenInExplorerCommand.Execute();
			}
		}
	}
}
