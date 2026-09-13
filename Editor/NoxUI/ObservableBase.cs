// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

namespace NoxUI;

	/// <summary>
	/// INotifyPropertyChanged の基底。CommunityToolkit.Mvvm の ObservableObject を包み、
	/// MVVM ツールキットへの依存を NoxUI の中に閉じ込める。
	/// ViewModel は <see cref="ViewModelBase"/> を、ViewModel でない通知オブジェクト
	/// (サービス等) はこのクラスを直接継承する。
	/// </summary>
	public abstract class ObservableBase : CommunityToolkit.Mvvm.ComponentModel.ObservableObject
	{
		/// <summary>
		/// 明示的な変更通知。ObservableObject の OnPropertyChanged と同じだが、
		/// 既存の呼び出し側 (Prism 時代の名前) をそのまま生かすために残している。
		/// </summary>
		protected void RaisePropertyChanged([System.Runtime.CompilerServices.CallerMemberName] string? propertyName = null)
		{
			OnPropertyChanged(propertyName);
		}
	}
