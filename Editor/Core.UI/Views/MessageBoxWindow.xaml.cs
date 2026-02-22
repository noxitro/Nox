using System.Windows;
using Core.UI.ViewModels;

namespace Core.UI.Views
{
    public partial class MessageBoxWindow : Window
    {
        public MessageBoxWindow()
        {
            InitializeComponent();
        }

		public MessageBoxWindow(MessageBoxWindowViewModel viewModel)
        {
            InitializeComponent();
            DataContext = viewModel;
            viewModel.CloseAction = Close;
		}
	}
}
