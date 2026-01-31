using System.Windows;
using Core.UI.ViewModels;

namespace Core.UI.Views
{
    public partial class MessageBoxWindow : Window
    {
		public MessageBoxWindow(ViewModels.MessageBoxViewModel viewModel)
		{
			InitializeComponent();
			DataContext = viewModel;
			Title = viewModel.Title;
		}

		public ViewModels.MessageBoxResult Result { get; private set; } = ViewModels.MessageBoxResult.Close;

		protected override void OnContentRendered(System.EventArgs e)
		{
			base.OnContentRendered(e);

			if (DataContext is MessageBoxViewModel vm)
			{
				vm.RequestClose += result =>
				{
					Result = result;
					DialogResult = true;
					Close();
				};
			}
		}
    }
}
