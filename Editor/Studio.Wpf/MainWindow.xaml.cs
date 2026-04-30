using System.Windows;
using Studio.Wpf.ViewModels;

namespace Studio.Wpf;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        DataContext = new MainWindowViewModel();
    }
}