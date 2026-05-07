using System.ComponentModel;
using AvalonDock.Themes;

namespace Studio.Wpf.Themes;

/// <summary>
/// アプリケーションテーマの取得を行うサービス。
/// 実装は <see cref="INotifyPropertyChanged"/> を介して
/// <see cref="CurrentTheme"/> 変更を通知すること。
/// </summary>
public interface IThemeService : INotifyPropertyChanged
{
    /// <summary>現在適用中の AvalonDock テーマ。</summary>
    Theme CurrentTheme { get; }
}
