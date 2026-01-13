using System;
using System.Collections.Generic;
using System.Text;

using System.Windows;

namespace Core.UI
{
	internal class WpfMessageService : Core.IMessageService
	{
		public void ShowMessage(string message, string? title = null, MessageLevel level = MessageLevel.Info)
		{
			// UI スレッドに切り替え
			Application.Current.Dispatcher.Invoke(() =>
			{
				var vm = new Core.UI.ViewModels.MessageBoxViewModel(
					message,
					title ?? GetDefaultTitle(level),
					ConvertLevel(level));

				var win = new Core.UI.Views.MessageBoxWindow
				{
					Owner = Application.Current.MainWindow,
					DataContext = vm
				};
				win.ShowDialog();
			});
		}

		private static string GetDefaultTitle(MessageLevel level) =>
			level switch
			{
				MessageLevel.Error => "Error",
				MessageLevel.Warning => "Warning",
				_ => "Information"
			};

		private static Core.UI.ViewModels.MessageBoxType ConvertLevel(MessageLevel level) =>
			level switch
			{
				MessageLevel.Error => Core.UI.ViewModels.MessageBoxType.Error,
				MessageLevel.Warning => Core.UI.ViewModels.MessageBoxType.Warning,
				_ => Core.UI.ViewModels.MessageBoxType.Info
			};
	}
}