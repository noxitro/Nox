using System;

namespace Core.UI.ViewModels
{
	public sealed class AssetPropertiesViewModel : NoxUI.ViewModelBase, IDisposable
	{
		#region Non-public fields
		private readonly Core.SelectionService _Selection;
		private bool _Disposed;
		#endregion

		#region Public properties
		public bool HasAsset
		{
			get => field;
			private set => SetProperty(ref field, value);
		}

		public string Icon
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = "";

		public string Name
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = "No asset selected";

		public string Summary
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = "Select an asset in the Asset Browser.";

		public string Guid
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public string Kind
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public string RelativePath
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public string FullPath
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public string SizeText
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public string LastWriteTime
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;
		#endregion

		public AssetPropertiesViewModel()
		{
			_Selection = Core.StudioManager.Instance.Workspace.Selection;
			_Selection.Changed += OnSelectionChanged;
			ApplySelection(_Selection.Current);
		}

		private void OnSelectionChanged(object? sender, EventArgs e)
		{
			System.Windows.Threading.Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (dispatcher != null && dispatcher.CheckAccess() == false)
			{
				NoxUI.DispatcherHelper.TryBeginInvoke(dispatcher, () => ApplySelection(_Selection.Current));
				return;
			}

			ApplySelection(_Selection.Current);
		}

		private void ApplySelection(Core.SelectionInfo selection)
		{
			if (selection.Value is Core.ProjectAsset asset)
			{
				HasAsset = true;
				Icon = GetAssetIcon(asset.Kind);
				Name = asset.Name;
				Summary = asset.RelativePath;
				Guid = asset.Guid;
				Kind = asset.Kind.ToString();
				RelativePath = asset.RelativePath;
				FullPath = asset.FullPath;
				SizeText = FormatSize(asset.Size);
				LastWriteTime = asset.LastWriteTime.ToString("yyyy/MM/dd HH:mm:ss");
				return;
			}

			HasAsset = false;
			Icon = "";
			Name = "No asset selected";
			Summary = "Select an asset in the Asset Browser.";
			Guid = string.Empty;
			Kind = string.Empty;
			RelativePath = string.Empty;
			FullPath = string.Empty;
			SizeText = string.Empty;
			LastWriteTime = string.Empty;
		}

		private static string GetAssetIcon(Core.AssetKind kind)
		{
			return kind switch
			{
				Core.AssetKind.Scene => "",
				Core.AssetKind.Model => "",
				Core.AssetKind.Texture => "",
				Core.AssetKind.Material => "",
				Core.AssetKind.Shader => "",
				Core.AssetKind.Script => "",
				Core.AssetKind.Audio => "",
				Core.AssetKind.Font => "",
				Core.AssetKind.Document => "",
				_ => "",
			};
		}

		private static string FormatSize(long size)
		{
			string[] units = ["B", "KB", "MB", "GB"];
			double value = size;
			int unitIndex = 0;
			while (value >= 1024 && unitIndex < units.Length - 1)
			{
				value /= 1024;
				++unitIndex;
			}

			return $"{value:0.#} {units[unitIndex]}";
		}

		public override void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			_Selection.Changed -= OnSelectionChanged;
			_Disposed = true;
		}
	}
}
