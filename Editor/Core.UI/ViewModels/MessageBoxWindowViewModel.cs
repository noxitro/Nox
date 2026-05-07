using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.ViewModels
{
	public class MessageBoxWindowViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public required string Title { get; init; } = "Title";
		public required string Message { get; init; } = "Message";
		public required System.Windows.MessageBoxImage Level { get; init; } = System.Windows.MessageBoxImage.Information;
		public required System.Windows.MessageBoxButton Style { get; init; } = System.Windows.MessageBoxButton.OK;
		public required Action<System.Windows.MessageBoxResult> Callback { get; init; } = (result) => { };

		public System.Windows.Media.Brush Background { get; } = System.Windows.SystemColors.GrayTextBrush;

		public NoxUI.ViewModelCommand YesCommand => field ??= new(ClickYes);
		public NoxUI.ViewModelCommand NoCommand => field ??= new(ClickNo);
		public NoxUI.ViewModelCommand CancelCommand => field ??= new(ClickCancel);

		public System.Windows.MessageBoxResult Result { get; private set; } = System.Windows.MessageBoxResult.Yes;
		public Action CloseAction { private get; set; } = () => { };
		#endregion

		#region 非公開メソッド
		public MessageBoxWindowViewModel()
		{
			switch(Level)
			{
				case System.Windows.MessageBoxImage.Error:
				//case System.Windows.MessageBoxImage.Hand:
				//case System.Windows.MessageBoxImage.Stop:
					Background = System.Windows.SystemColors.ControlDarkBrush;
					break;
				case System.Windows.MessageBoxImage.Warning:
					Background = System.Windows.SystemColors.ControlDarkBrush;
					break;
				case System.Windows.MessageBoxImage.Information:
				//case System.Windows.MessageBoxImage.Asterisk:
					Background = System.Windows.SystemColors.ControlLightBrush;
					break;
				case System.Windows.MessageBoxImage.Question:
					Background = System.Windows.SystemColors.ControlLightBrush;
					break;
				default:
					Background = System.Windows.SystemColors.ControlBrush;
					break;
			}
		}

		private void ClickYes()
		{
			Clicked(System.Windows.MessageBoxResult.Yes);
		}

		private void ClickNo()
		{
			Clicked(System.Windows.MessageBoxResult.No);
		}

		private void ClickCancel()
		{
			Clicked(System.Windows.MessageBoxResult.Cancel);
		}

		private void Clicked(System.Windows.MessageBoxResult result)
		{
			Callback(result);
			Result = result;
			CloseAction();
		}
		#endregion
	}
}
