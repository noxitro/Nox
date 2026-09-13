using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.ViewModels;

	public class MessageBoxWindowViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public required string Title { get; init; } = "Title";
		public required string Message { get; init; } = "Message";
		public required System.Windows.MessageBoxImage Level { get; init; } = System.Windows.MessageBoxImage.Information;
		public required System.Windows.MessageBoxButton Style { get; init; } = System.Windows.MessageBoxButton.OK;
		public required Action<System.Windows.MessageBoxResult> Callback { get; init; } = (result) => { };

		public string PrimaryButtonText => Style is System.Windows.MessageBoxButton.OK or System.Windows.MessageBoxButton.OKCancel
			? "OK"
			: "Yes";
		public NoxUI.ViewModelCommand PrimaryCommand => field ??= new(ClickPrimary);
		public NoxUI.ViewModelCommand NoCommand => field ??= new(ClickNo);
		public NoxUI.ViewModelCommand CancelCommand => field ??= new(ClickCancel);

		public System.Windows.MessageBoxResult Result { get; private set; } = System.Windows.MessageBoxResult.None;
		public Action CloseAction { private get; set; } = () => { };
		#endregion

		#region 非公開メソッド
		private void ClickPrimary()
		{
			Clicked(Style is System.Windows.MessageBoxButton.OK or System.Windows.MessageBoxButton.OKCancel
				? System.Windows.MessageBoxResult.OK
				: System.Windows.MessageBoxResult.Yes);
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

		public bool TryCloseFromWindow()
		{
			if (Result != System.Windows.MessageBoxResult.None)
			{
				return true;
			}

			System.Windows.MessageBoxResult closeResult = Style switch
			{
				System.Windows.MessageBoxButton.OK => System.Windows.MessageBoxResult.OK,
				System.Windows.MessageBoxButton.OKCancel => System.Windows.MessageBoxResult.Cancel,
				System.Windows.MessageBoxButton.YesNoCancel => System.Windows.MessageBoxResult.Cancel,
				_ => System.Windows.MessageBoxResult.None,
			};
			if (closeResult == System.Windows.MessageBoxResult.None)
			{
				return false;
			}

			Result = closeResult;
			Callback(closeResult);
			return true;
		}
		#endregion
	}
