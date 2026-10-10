// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.IO;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Studio.Wpf;

/// <summary>
/// ウィンドウの見た目を PNG に保存する。
/// CI の UI テストが PR 用のスクリーンショットを集めるための開発用の口で、
/// 環境変数 <see cref="DirectoryVariable"/> が設定されているときだけ使う。
/// 画面からの取り込みではなくビジュアルツリーの描画なので、ランナーの画面解像度やウィンドウの位置に左右されない。
/// </summary>
internal static class WindowScreenshot
{
    public const string DirectoryVariable = "NOX_STUDIO_SCREENSHOT_DIR";

    /// <summary>保存先のフォルダー。環境変数が無ければ null で、保存は行わない。</summary>
    public static string? ResolveDirectory()
    {
        string? directory = Environment.GetEnvironmentVariable(DirectoryVariable);
        return string.IsNullOrWhiteSpace(directory) ? null : directory;
    }

    /// <summary>
    /// ウィンドウの内容を論理ピクセル等倍で描画し、<c>directory/name.png</c> に保存する。
    /// 一時ファイルに書いてから置き換えるので、読む側が書きかけのファイルを拾わない。
    /// </summary>
    public static void Save(Window window, string directory, string name)
    {
        if (window.Content is not FrameworkElement root)
        {
            return;
        }

        int width = (int)Math.Ceiling(root.ActualWidth);
        int height = (int)Math.Ceiling(root.ActualHeight);
        if (width <= 0 || height <= 0)
        {
            return;
        }

        // Visual を直接 Render すると親からのオフセットが乗るので、VisualBrush で原点に敷いて描く
        DrawingVisual drawing = new();
        using (DrawingContext context = drawing.RenderOpen())
        {
            context.DrawRectangle(new VisualBrush(root), null, new Rect(0, 0, width, height));
        }

        RenderTargetBitmap bitmap = new(width, height, 96, 96, PixelFormats.Pbgra32);
        bitmap.Render(drawing);

        PngBitmapEncoder encoder = new();
        encoder.Frames.Add(BitmapFrame.Create(bitmap));

        Directory.CreateDirectory(directory);
        string path = Path.Combine(directory, name + ".png");
        string temporaryPath = path + ".tmp";
        using (FileStream stream = File.Create(temporaryPath))
        {
            encoder.Save(stream);
        }
        File.Move(temporaryPath, path, overwrite: true);
    }
}
