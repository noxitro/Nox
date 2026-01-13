using System.Windows;
using Core.UI.ViewModels;

namespace Core.UI.Views
{
    public partial class MessageBoxWindow : Window
    {
        public MessageBoxWindow()
        {
            InitializeComponent();

            DataContext = NoxUI.PrismHelper.ResolveDataContext<MessageBoxViewModel>();
		}

        protected override void OnContentRendered(System.EventArgs e)
        {
            base.OnContentRendered(e);

            if (DataContext is MessageBoxViewModel vm)
            {
                vm.RequestClose += () =>
                {
                    DialogResult = true;
                    Close();
                };
            }
        }
    }
}
