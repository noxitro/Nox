using System.Windows;
using System.Windows.Input;
using System.ComponentModel;
using Core.UI.ViewModels;

namespace Core.UI.Views;

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

    private void TitleBar_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        DragMove();
    }

    private void CloseButton_Click(object sender, RoutedEventArgs e)
    {
        Close();
    }

	private void OnWindowClosing(object? sender, CancelEventArgs e)
	{
		if (DataContext is MessageBoxWindowViewModel viewModel)
		{
			e.Cancel = viewModel.TryCloseFromWindow() == false;
		}
	}
	}
