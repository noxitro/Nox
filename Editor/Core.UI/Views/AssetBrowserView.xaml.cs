// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Windows;
using System.Windows.Media;
using System.Windows.Threading;

namespace Core.UI.Views;

	/// <summary>
	/// AssetBrowserView.xaml の相互作用ロジック
	/// </summary>
	public partial class AssetBrowserView : System.Windows.Controls.UserControl
	{
		public AssetBrowserView()
		{
			InitializeComponent();
		}

		private void OnSelectedItemChanged(object sender, System.Windows.RoutedPropertyChangedEventArgs<object> e)
		{
			if (DataContext is Core.UI.ViewModels.AssetBrowserViewModel viewModel)
			{
				viewModel.SelectedTreeNode = e.NewValue as Core.UI.ViewModels.AssetTreeNodeViewModel;
			}
		}

		private void OnTreeViewItemPreviewMouseRightButtonDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (sender is not System.Windows.Controls.TreeViewItem { DataContext: Core.UI.ViewModels.AssetTreeNodeViewModel node } item)
			{
				return;
			}

			item.Focus();
			item.IsSelected = true;
			if (DataContext is Core.UI.ViewModels.AssetBrowserViewModel viewModel)
			{
				viewModel.SelectedAsset = null;
				viewModel.SelectedTreeNode = node;
			}
		}

		private void OnAssetItemPreviewMouseRightButtonDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (DataContext is not Core.UI.ViewModels.AssetBrowserViewModel viewModel)
			{
				return;
			}

			switch (sender)
			{
				case System.Windows.Controls.ListViewItem { DataContext: Core.UI.ViewModels.ProjectAssetViewModel asset } item:
					item.Focus();
					item.IsSelected = true;
					viewModel.SelectedAsset = asset;
					break;
				case System.Windows.Controls.ListBoxItem { DataContext: Core.UI.ViewModels.ProjectAssetViewModel asset } item:
					item.Focus();
					item.IsSelected = true;
					viewModel.SelectedAsset = asset;
					break;
			}
		}

		private void OnAssetDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (DataContext is Core.UI.ViewModels.AssetBrowserViewModel viewModel &&
				viewModel.SelectedAsset is { IsRenaming: false } &&
				viewModel.OpenInExplorerCommand.CanExecute())
			{
				viewModel.OpenInExplorerCommand.Execute();
			}
		}

		private void OnAssetBrowserPreviewKeyDown(object sender, System.Windows.Input.KeyEventArgs e)
		{
			if (e.Key == System.Windows.Input.Key.Enter &&
				e.OriginalSource is not System.Windows.Controls.TextBox &&
				sender is System.Windows.Controls.ListView or System.Windows.Controls.ListBox &&
				DataContext is Core.UI.ViewModels.AssetBrowserViewModel { SelectedAsset.IsRenaming: false } openViewModel &&
				openViewModel.OpenInExplorerCommand.CanExecute())
			{
				openViewModel.OpenInExplorerCommand.Execute();
				e.Handled = true;
				return;
			}

			if (e.Key != System.Windows.Input.Key.F2 ||
				e.OriginalSource is System.Windows.Controls.TextBox ||
				DataContext is not Core.UI.ViewModels.AssetBrowserViewModel viewModel)
			{
				return;
			}

			switch (sender)
			{
				case System.Windows.Controls.TreeView when viewModel.RenameFolderCommand.CanExecute():
					viewModel.RenameFolderCommand.Execute();
					e.Handled = true;
					break;
				case System.Windows.Controls.ListView when viewModel.RenameAssetCommand.CanExecute():
					viewModel.RenameAssetCommand.Execute();
					e.Handled = true;
					break;
				case System.Windows.Controls.ListBox when viewModel.RenameAssetCommand.CanExecute():
					viewModel.RenameAssetCommand.Execute();
					e.Handled = true;
					break;
			}
		}

		private void OnAssetBrowserPreviewMouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (DataContext is not Core.UI.ViewModels.AssetBrowserViewModel viewModel ||
				IsRenameEditorHit(e.OriginalSource as DependencyObject))
			{
				return;
			}

			viewModel.CompleteActiveRenameOnFocusLoss();
		}

		private void OnRenameEditorLoaded(object sender, System.Windows.RoutedEventArgs e)
		{
			if (sender is not System.Windows.Controls.TextBox editor)
			{
				return;
			}

			NoxUI.DispatcherHelper.TryBeginInvoke(editor.Dispatcher, () =>
			{
				if (editor.Visibility != System.Windows.Visibility.Visible)
				{
					return;
				}

				editor.Focus();
				switch (editor.DataContext)
				{
					case Core.UI.ViewModels.ProjectAssetViewModel asset when string.IsNullOrWhiteSpace(asset.Extension) == false &&
						editor.Text.EndsWith(asset.Extension, StringComparison.OrdinalIgnoreCase):
						editor.Select(0, Math.Max(0, editor.Text.Length - asset.Extension.Length));
						break;
					default:
						editor.SelectAll();
						break;
				}
			}, DispatcherPriority.Input);
		}

		private void OnRenameEditorKeyDown(object sender, System.Windows.Input.KeyEventArgs e)
		{
			if (sender is not System.Windows.Controls.TextBox editor || DataContext is not Core.UI.ViewModels.AssetBrowserViewModel viewModel)
			{
				return;
			}

			switch (e.Key)
			{
				case System.Windows.Input.Key.Enter:
					CommitRename(editor, viewModel);
					e.Handled = true;
					break;
				case System.Windows.Input.Key.Escape:
					if (editor.DataContext != null)
					{
						viewModel.CancelRename(editor.DataContext);
					}

					e.Handled = true;
					break;
			}
		}

		private void OnRenameEditorLostFocus(object sender, RoutedEventArgs e)
		{
			if (sender is System.Windows.Controls.TextBox editor && DataContext is Core.UI.ViewModels.AssetBrowserViewModel viewModel)
			{
				CompleteRenameOnFocusLoss(editor, viewModel);
			}
		}

		private static void CommitRename(System.Windows.Controls.TextBox editor, Core.UI.ViewModels.AssetBrowserViewModel viewModel)
		{
			if (editor.DataContext == null)
			{
				return;
			}

			viewModel.CommitRename(editor.DataContext);
			switch (editor.DataContext)
			{
				case Core.UI.ViewModels.ProjectAssetViewModel asset when asset.IsRenaming:
					RefocusRenameEditor(editor, asset.Extension);
					break;
				case Core.UI.ViewModels.AssetTreeNodeViewModel node when node.IsRenaming:
					RefocusRenameEditor(editor, extension: string.Empty);
					break;
			}
		}

		private static void CompleteRenameOnFocusLoss(System.Windows.Controls.TextBox editor, Core.UI.ViewModels.AssetBrowserViewModel viewModel)
		{
			if (editor.DataContext == null)
			{
				return;
			}

			viewModel.CompleteRenameOnFocusLoss(editor.DataContext);
		}

		private static void RefocusRenameEditor(System.Windows.Controls.TextBox editor, string extension)
		{
			NoxUI.DispatcherHelper.TryBeginInvoke(editor.Dispatcher, () =>
			{
				if (editor.IsLoaded == false || editor.Visibility != System.Windows.Visibility.Visible)
				{
					return;
				}

				editor.Focus();
				if (string.IsNullOrWhiteSpace(extension) == false &&
					editor.Text.EndsWith(extension, StringComparison.OrdinalIgnoreCase))
				{
					editor.Select(0, Math.Max(0, editor.Text.Length - extension.Length));
					return;
				}

				editor.SelectAll();
			}, DispatcherPriority.Input);
		}

		private static bool IsRenameEditorHit(DependencyObject? source)
		{
			System.Windows.Controls.TextBox? editor = FindVisualParent<System.Windows.Controls.TextBox>(source);
			return editor?.DataContext is Core.UI.ViewModels.ProjectAssetViewModel or Core.UI.ViewModels.AssetTreeNodeViewModel;
		}

		private static T? FindVisualParent<T>(DependencyObject? source) where T : DependencyObject
		{
			while (source != null)
			{
				if (source is T target)
				{
					return target;
				}

				source = VisualTreeHelper.GetParent(source);
			}

			return null;
		}
	}
