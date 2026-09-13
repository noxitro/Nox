// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI;

	public static class MessageBox
	{
		#region 公開メソッド
		public static void Show(
			string message,
			string title = "title",
			System.Windows.MessageBoxImage level = System.Windows.MessageBoxImage.Information,
			System.Windows.MessageBoxButton style = System.Windows.MessageBoxButton.OK,
			Action<System.Windows.MessageBoxResult>? callback = null
			)
		{
			Core.UI.MessageServiceProvider.Current.Show(new IMessageService.Args
			{
				Title = title,
				Message = message,
				Level = level,
				Style = style,
				Callback = callback ?? (_ => { }),
			});
		}

		public static System.Windows.MessageBoxResult ShowDialog(string message,
			string title = "title",
			System.Windows.MessageBoxImage level = System.Windows.MessageBoxImage.Information,
			System.Windows.MessageBoxButton style = System.Windows.MessageBoxButton.OK)
		{
			return Core.UI.MessageServiceProvider.Current.ShowDialog(new IMessageService.Args
			{
				Title = title,
				Message = message,
				Level = level,
				Style = style,
				Callback = (_ => { }),
			});
		}
		#endregion
	}

	public interface IMessageService
	{
		public readonly struct Args
		{
			public required string Message { get; init; }
			public required string Title { get; init; }
			public required System.Windows.MessageBoxImage Level { get; init; }
			public required System.Windows.MessageBoxButton Style { get; init; }
			public required Action<System.Windows.MessageBoxResult> Callback { get; init; }

			public bool ShowDialog { get; init; } = true;

			public Args() { }
		}

		void Show(in Args args);
		System.Windows.MessageBoxResult ShowDialog(in Args args);
	}

	public static class MessageServiceProvider
	{
		private static IMessageService? _Current = null;
		public static IMessageService Current
		{
			get
			{
				if (_Current == null)
				{
					throw new InvalidOperationException("MessageService is not registered.");
				}
				return _Current;
			}
		}
		public static void Register(IMessageService service)
		{
			_Current = service;
		}
	}

	internal class WpfMessageService : Core.UI.IMessageService
	{
		void Core.UI.IMessageService.Show(in Core.UI.IMessageService.Args args)
		{
			Core.UI.ViewModels.MessageBoxWindowViewModel vm = new()
			{
				Message = args.Message,
				Title = args.Title,
				Level = args.Level,
				Style = args.Style,
				Callback = args.Callback,
			};

			System.Windows.Application.Current.Dispatcher.Invoke(() => {
				Views.MessageBoxWindow win = new(vm)
				{
					Owner = System.Windows.Application.Current.MainWindow,
					WindowStartupLocation = System.Windows.WindowStartupLocation.CenterOwner,
				};

				win.Show();
			});
		}

		System.Windows.MessageBoxResult IMessageService.ShowDialog(in IMessageService.Args args)
		{
			Core.UI.ViewModels.MessageBoxWindowViewModel vm = new()
			{
				Message = args.Message,
				Title = args.Title,
				Level = args.Level,
				Style = args.Style,
				Callback = args.Callback,
			};

			System.Windows.Application.Current.Dispatcher.Invoke(() => 
			{
				Views.MessageBoxWindow win = new(vm)
				{
					Owner = System.Windows.Application.Current.MainWindow,
					WindowStartupLocation = System.Windows.WindowStartupLocation.CenterOwner,
				};
				bool? result = win.ShowDialog();
			});

			return vm.Result;
		}
	}