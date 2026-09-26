// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace Core.UI.Views;

	/// <summary>
	/// HierarchyView.xaml の相互作用ロジック
	/// </summary>
	public partial class HierarchyView : System.Windows.Controls.UserControl
	{
		public HierarchyView()
		{
			InitializeComponent();
		}

		private void OnSelectedItemChanged(object sender, System.Windows.RoutedPropertyChangedEventArgs<object> e)
		{
			if (DataContext is Core.UI.ViewModels.HierarchyViewModel viewModel)
			{
				viewModel.SelectedNode = e.NewValue as Core.UI.ViewModels.HierarchyNodeViewModel;
			}
		}

		private void OnTreeViewItemPreviewMouseRightButtonDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (FindVisualParent<TreeViewItem>(e.OriginalSource as DependencyObject) is { } item)
			{
				item.Focus();
				item.IsSelected = true;
			}
		}

		private void OnHierarchyPreviewKeyDown(object sender, System.Windows.Input.KeyEventArgs e)
		{
			if (e.Key == System.Windows.Input.Key.Delete &&
				e.OriginalSource is not System.Windows.Controls.TextBox &&
				DataContext is Core.UI.ViewModels.HierarchyViewModel deleteViewModel &&
				deleteViewModel.DeleteSelectedCommand.CanExecute())
			{
				deleteViewModel.DeleteSelectedCommand.Execute();
				e.Handled = true;
				return;
			}

			if (e.Key != System.Windows.Input.Key.F2 ||
				e.OriginalSource is System.Windows.Controls.TextBox ||
				DataContext is not Core.UI.ViewModels.HierarchyViewModel viewModel)
			{
				return;
			}

			if (viewModel.RenameSelectedCommand.CanExecute())
			{
				viewModel.RenameSelectedCommand.Execute();
				e.Handled = true;
			}
		}

		private void OnHierarchyPreviewMouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (DataContext is not Core.UI.ViewModels.HierarchyViewModel viewModel ||
				IsRenameEditorHit(e.OriginalSource as DependencyObject))
			{
				return;
			}

			viewModel.CompleteActiveRenameOnFocusLoss();
		}

		private void OnRenameEditorLoaded(object sender, RoutedEventArgs e)
		{
			if (sender is not System.Windows.Controls.TextBox editor)
			{
				return;
			}

			NoxUI.DispatcherHelper.TryBeginInvoke(editor.Dispatcher, () =>
			{
				if (editor.Visibility != Visibility.Visible)
				{
					return;
				}

				editor.Focus();
				editor.SelectAll();
			}, DispatcherPriority.Input);
		}

		private void OnRenameEditorKeyDown(object sender, System.Windows.Input.KeyEventArgs e)
		{
			if (sender is not System.Windows.Controls.TextBox editor || DataContext is not Core.UI.ViewModels.HierarchyViewModel viewModel)
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
			if (sender is System.Windows.Controls.TextBox editor && DataContext is Core.UI.ViewModels.HierarchyViewModel viewModel)
			{
				CompleteRenameOnFocusLoss(editor, viewModel);
			}
		}

		private static void CommitRename(System.Windows.Controls.TextBox editor, Core.UI.ViewModels.HierarchyViewModel viewModel)
		{
			if (editor.DataContext == null)
			{
				return;
			}

			viewModel.CommitRename(editor.DataContext);
			if (editor.DataContext is Core.UI.ViewModels.HierarchyNodeViewModel node && node.IsRenaming)
			{
				NoxUI.DispatcherHelper.TryBeginInvoke(editor.Dispatcher, () =>
				{
					if (editor.IsLoaded == false || editor.Visibility != Visibility.Visible)
					{
						return;
					}

					editor.Focus();
					editor.SelectAll();
				}, DispatcherPriority.Input);
			}
		}

		private static void CompleteRenameOnFocusLoss(System.Windows.Controls.TextBox editor, Core.UI.ViewModels.HierarchyViewModel viewModel)
		{
			if (editor.DataContext == null)
			{
				return;
			}

			viewModel.CompleteRenameOnFocusLoss(editor.DataContext);
		}

		private static bool IsRenameEditorHit(DependencyObject? source)
		{
			System.Windows.Controls.TextBox? editor = FindVisualParent<System.Windows.Controls.TextBox>(source);
			return editor?.DataContext is Core.UI.ViewModels.HierarchyNodeViewModel;
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
