// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Windows.Input;

namespace NoxUI;

	/// <summary>
	/// ICommand 実装。CommunityToolkit.Mvvm の RelayCommand を内包し、
	/// MVVM ツールキットへの依存を NoxUI の中に閉じ込める。
	/// RelayCommand は sealed なので継承ではなく委譲で包む。
	/// </summary>
	public class ViewModelCommand : ICommand
	{
		private readonly CommunityToolkit.Mvvm.Input.RelayCommand _Inner;

		public ViewModelCommand(Action executeMethod)
		{
			_Inner = new(executeMethod);
		}

		public ViewModelCommand(Action executeMethod, Func<bool> canExecuteMethod)
		{
			_Inner = new(executeMethod, canExecuteMethod);
		}

		public event EventHandler? CanExecuteChanged
		{
			add => _Inner.CanExecuteChanged += value;
			remove => _Inner.CanExecuteChanged -= value;
		}

		public bool CanExecute(object? parameter) => _Inner.CanExecute(parameter);

		public void Execute(object? parameter) => _Inner.Execute(parameter);

		/// <summary>引数を取らないコマンド向けの短縮形。</summary>
		public bool CanExecute() => _Inner.CanExecute(null);

		/// <summary>引数を取らないコマンド向けの短縮形。</summary>
		public void Execute() => _Inner.Execute(null);

		/// <summary>CanExecute の再評価を要求する。RelayCommand.NotifyCanExecuteChanged と同じ。</summary>
		public void RaiseCanExecuteChanged() => _Inner.NotifyCanExecuteChanged();
	}
