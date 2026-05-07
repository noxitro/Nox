using System;
using System.Collections.Generic;
using System.Text;

namespace Core
{
	namespace Attributes
	{
		/// <summary>
		/// メタ情報
		/// </summary>
		public sealed class MetaAttribute : System.Attribute
		{
		}
	}

	public abstract class Asset
	{
		#region 公開プロパティ
		[Core.Attributes.Meta]
		public System.Uri Uri { get; set; } = new System.Uri("assets:/");

        /// <summary>
        /// 第一拡張子は.jsonや、.fbxなどのファイル形式を表すものとする。
		/// 第二拡張子は、studioで識別するためのもので、.scnや.meshなどのものとする。
		/// AssetTypeが返すのは、第二拡張子の方とする。
        /// </summary>
        public string AssetType
		{
			get
			{
				string extension = System.IO.Path.GetExtension(Uri.AbsolutePath);
				if (extension.StartsWith("."))
				{
					return extension.Substring(1);
				}
				else
				{
					return extension;
                }
            }
		}
		#endregion

		#region 公開メソッド
		public void Convert()
		{
			OnConvert();
		}
		#endregion

		#region 非公開メソッド
		public virtual void OnConvert() { }
		#endregion
	}

    public enum AssetKind : byte
	{
		Unknown,
		Folder,
		Scene,
		Model,
		Texture,
		Material,
		Shader,
		Script,
		Audio,
		Font,
		Document,
	}

	public sealed class ProjectAsset : Asset
	{
		#region 公開プロパティ
		public string Guid { get; init; } = string.Empty;
		public string Name { get; init; } = string.Empty;
		public string FullPath { get; init; } = string.Empty;
		public string MetaPath { get; init; } = string.Empty;
		public string RelativePath { get; init; } = string.Empty;
		public string Extension { get; init; } = string.Empty;
		public AssetKind Kind { get; init; } = AssetKind.Unknown;
		public long Size { get; init; }
		public DateTime LastWriteTime { get; init; }
		#endregion
	}

	public sealed class AssetMeta
	{
		#region 公開プロパティ
		public int Version { get; set; } = 1;
		public string Guid { get; set; } = System.Guid.NewGuid().ToString("N");
		public string AssetPath { get; set; } = string.Empty;
		public AssetKind Kind { get; set; } = AssetKind.Unknown;
		public string Importer { get; set; } = "DefaultImporter";
		public DateTime CreatedAtUtc { get; set; } = DateTime.UtcNow;
		public DateTime UpdatedAtUtc { get; set; } = DateTime.UtcNow;
		#endregion
	}

	public sealed class AssetTreeNode
	{
		#region 非公開フィールド
		private readonly List<AssetTreeNode> _Children = new();
		#endregion

		#region 公開プロパティ
		public string Name { get; }
		public string RelativePath { get; }
		public ProjectAsset? Asset { get; }
		public IReadOnlyList<AssetTreeNode> Children => _Children;
		public bool IsFolder => Asset == null || Asset.Kind == AssetKind.Folder;
		#endregion

		public AssetTreeNode(string name, string relativePath, ProjectAsset? asset)
		{
			Name = name;
			RelativePath = relativePath;
			Asset = asset;
		}

		public AssetTreeNode AddChild(AssetTreeNode child)
		{
			_Children.Add(child);
			return child;
		}
	}
}
