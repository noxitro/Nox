using System;
using System.Collections.Generic;

namespace Core.UI.ViewModels;

	public class DocumentTabViewModel : NoxUI.ViewModelBase
	{
		#region 型定義
		public class DocumentTabChildItem
		{
			public required string Header { get; init; }
			public string? Glyph { get; init; }
			public string? Tooltip { get; init; }
			public NoxUI.ViewModelCommand? Command { get; init; }
			public IReadOnlyList<DocumentTabChildItem> Children { get; init; } = Array.Empty<DocumentTabChildItem>();
			public bool HasChildren => Children.Count > 0;
		}

		public class DocumentTabItem
		{
			public required string Header { get; init; }
			public IReadOnlyList<DocumentTabChildItem> Children { get; init; } = Array.Empty<DocumentTabChildItem>();
			public bool HasChildren => Children.Count > 0;
			public string? Tooltip { get; init; }
			public NoxUI.ViewModelCommand? Command { get; init; }
		}
		#endregion

		#region 非公開フィールド
		private readonly DocumentTabItem[] _TabTable;
		#endregion

		#region コンストラクタ
		public DocumentTabViewModel()
		{
			_TabTable = new[]
			{
				new DocumentTabItem
				{
					Header = "Views",
					Tooltip = "ビューを表示",
					Children = new DocumentTabChildItem[]
					{
						new()
						{
							Header = "Core",
							Tooltip = "コアビューを表示",
							Children = new DocumentTabChildItem[]
							{
								new() { Header = "Inspector", Tooltip = "Inspector を表示", Command = ShowInspectorCommand },
								new() { Header = "Hierarchy", Tooltip = "Hierarchy を表示", Command = ShowHierarchyCommand },
								new() { Header = "Project Settings", Tooltip = "Project Settings を表示", Command = ShowProjectSettingsCommand },
							},
						},
					},
				},
				new DocumentTabItem
				{
					Header = "Develop",
					Tooltip = "開発ツール",
					Children = new DocumentTabChildItem[]
					{
						new()
						{
							Header = "GenerateRemoteCode",
							Glyph = "\uE7BE",
							Tooltip = "RemoteCodeを出力します",
							Command = GenerateRemoteCodeCommand,
						},
					},
				},
			};
		}
		#endregion

		#region 公開プロパティ
		public IReadOnlyList<DocumentTabItem> TabTable => _TabTable;

		public NoxUI.ViewModelCommand GenerateRemoteCodeCommand => field ??= new(GenerateRemoteCode);
		public NoxUI.ViewModelCommand ShowInspectorCommand => field ??= new(ShowInspector);
		public NoxUI.ViewModelCommand ShowHierarchyCommand => field ??= new(ShowHierarchy);
		public NoxUI.ViewModelCommand ShowProjectSettingsCommand => field ??= new(ShowProjectSettings);
		#endregion

		#region 非公開メソッド
		private void GenerateRemoteCode()
		{
			var result = Core.UI.MessageBox.ShowDialog(
				"RemoteCodeを出力しますか？",
				"確認",
				System.Windows.MessageBoxImage.Information,
				System.Windows.MessageBoxButton.YesNo
				);

			if (result == System.Windows.MessageBoxResult.Yes)
			{
				Core.RuntimeRemote.RuntimeRemoteCodeGenerator generator = new();
				generator.GenerateCode();

				Core.UI.MessageBox.ShowDialog(
					"RemoteCodeの出力が完了しました。",
					"情報",
					System.Windows.MessageBoxImage.Information,
					System.Windows.MessageBoxButton.OK
					);
			}
		}

		private void ShowInspector()
		{
			// TODO: Inspector の表示処理を実装
		}

		private void ShowHierarchy()
		{
			Core.StudioManager.Instance.Workspace.SceneHierarchy.InitializeDefaultScene();
		}

		private void ShowProjectSettings()
		{
			Core.UI.ProjectSettingsViewService.Show();
		}
		#endregion
	}
