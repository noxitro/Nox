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
			ShowMessage(message, title, level, Core.UI.ViewModels.MessageBoxStyle.Yes);
		}

		public void ShowMessage(string message, string? title, MessageLevel level, Core.UI.ViewModels.MessageBoxStyle style)
		{
			_ = ShowMessageWithResult(message, title, level, style);
		}

		public Core.UI.ViewModels.MessageBoxResult ShowMessageWithResult(string message, string? title = null, MessageLevel level = MessageLevel.Info, Core.UI.ViewModels.MessageBoxStyle style = Core.UI.ViewModels.MessageBoxStyle.Yes)
		{
			var result = Core.UI.ViewModels.MessageBoxResult.Close;
			// UI スレッドに切り替え
			Application.Current.Dispatcher.Invoke(() =>
			{
				var vm = new Core.UI.ViewModels.MessageBoxViewModel(
					message,
					title ?? GetDefaultTitle(level),
					ConvertLevel(level),
					style);

				var win = new Core.UI.Views.MessageBoxWindow(vm)
				{
					Owner = Application.Current.MainWindow,
				};
				win.ShowDialog();
				result = win.Result;
			});
			return result;
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

	public static class MessageBox
	{
		public static Core.UI.ViewModels.MessageBoxResult Show(string message, string? title = null, ViewModels.MessageBoxType type = ViewModels.MessageBoxType.Info, ViewModels.MessageBoxStyle style = ViewModels.MessageBoxStyle.Yes)
		{
			if (Core.MessageServiceProvider.Current is WpfMessageService svc)
			{
				return svc.ShowMessageWithResult(message, title ?? GetDefaultTitle(type), ConvertLevel(type), style);
			}

			Core.MessageServiceProvider.Current.ShowMessage(message, title ?? GetDefaultTitle(type), ConvertLevel(type));
			return ViewModels.MessageBoxResult.Close;
		}

		private static string GetDefaultTitle(ViewModels.MessageBoxType type) =>
			type switch
			{
				ViewModels.MessageBoxType.Error => "Error",
				ViewModels.MessageBoxType.Warning => "Warning",
				_ => "Information"
			};

		private static MessageLevel ConvertLevel(ViewModels.MessageBoxType type) =>
			type switch
			{
				ViewModels.MessageBoxType.Error => MessageLevel.Error,
				ViewModels.MessageBoxType.Warning => MessageLevel.Warning,
				_ => MessageLevel.Info
			};
	}
}