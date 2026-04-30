using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace Studio.Wpf.Themes;

/// <summary>
/// アプリケーションテーマの取得・切替を行うサービス。
/// 実装は <see cref="INotifyPropertyChanged"/> を介して
/// <see cref="CurrentTheme"/> 変更を通知すること。
/// </summary>
public interface IThemeService : INotifyPropertyChanged
{
    /// <summary>現在適用中のテーマ。</summary>
    MetallicThemeBase CurrentTheme { get; }

    /// <summary>選択可能なテーマの一覧。</summary>
    IReadOnlyList<MetallicThemeBase> AvailableThemes { get; }

    /// <summary>テーマを切り替える。</summary>
    void SetTheme(MetallicThemeKind kind);
}
