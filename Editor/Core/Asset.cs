// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;

namespace Core;

	public abstract class Asset
	{
		#region 公開プロパティ
		[Core.Attributes.Meta]
		public System.Uri Uri { get; set; } = new System.Uri("assets:/");

		/// <summary>
		/// ファイル拡張子から算出したアセットタイプを返します。
		/// 先頭のドットは含みません。
		/// </summary>
		public string AssetType => AssetTypeUtility.GetAssetType(System.IO.Path.GetExtension(Uri.AbsolutePath));
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

	public static class AssetTypeUtility
	{
		public static string GetAssetType(string extension)
		{
			if (string.IsNullOrEmpty(extension))
			{
				return string.Empty;
			}

			return extension[0] == '.'
				? extension[1..]
				: extension;
		}

		public static string GetImporterName(string extension, bool isFolder = false)
		{
			if (isFolder)
			{
				return "FolderImporter";
			}

			if (HasExtension(extension, ".noxscene") || HasExtension(extension, ".scene"))
			{
				return "SceneImporter";
			}

			if (HasExtension(extension, ".fbx") ||
				HasExtension(extension, ".obj") ||
				HasExtension(extension, ".gltf") ||
				HasExtension(extension, ".glb"))
			{
				return "ModelImporter";
			}

			if (HasExtension(extension, ".png") ||
				HasExtension(extension, ".jpg") ||
				HasExtension(extension, ".jpeg") ||
				HasExtension(extension, ".tga") ||
				HasExtension(extension, ".bmp") ||
				HasExtension(extension, ".dds"))
			{
				return "TextureImporter";
			}

			if (HasExtension(extension, ".mat") || HasExtension(extension, ".material"))
			{
				return "MaterialImporter";
			}

			if (HasExtension(extension, ".hlsl") || HasExtension(extension, ".fx") || HasExtension(extension, ".shader"))
			{
				return "ShaderImporter";
			}

			if (HasExtension(extension, ".cs") ||
				HasExtension(extension, ".cpp") ||
				HasExtension(extension, ".h") ||
				HasExtension(extension, ".hpp"))
			{
				return "ScriptImporter";
			}

			if (HasExtension(extension, ".wav") || HasExtension(extension, ".mp3") || HasExtension(extension, ".ogg"))
			{
				return "AudioImporter";
			}

			if (HasExtension(extension, ".ttf") || HasExtension(extension, ".otf"))
			{
				return "FontImporter";
			}

			if (HasExtension(extension, ".md") ||
				HasExtension(extension, ".txt") ||
				HasExtension(extension, ".json") ||
				HasExtension(extension, ".xml") ||
				HasExtension(extension, ".yaml") ||
				HasExtension(extension, ".yml"))
			{
				return "TextImporter";
			}

			return "DefaultImporter";
		}

		public static string GetIconGlyph(string extension, bool isFolder = false)
		{
			if (isFolder)
			{
				return "";
			}

			if (HasExtension(extension, ".noxscene") || HasExtension(extension, ".scene"))
			{
				return "";
			}

			if (HasExtension(extension, ".fbx") ||
				HasExtension(extension, ".obj") ||
				HasExtension(extension, ".gltf") ||
				HasExtension(extension, ".glb"))
			{
				return "";
			}

			if (HasExtension(extension, ".png") ||
				HasExtension(extension, ".jpg") ||
				HasExtension(extension, ".jpeg") ||
				HasExtension(extension, ".tga") ||
				HasExtension(extension, ".bmp") ||
				HasExtension(extension, ".dds"))
			{
				return "";
			}

			if (HasExtension(extension, ".mat") || HasExtension(extension, ".material"))
			{
				return "";
			}

			if (HasExtension(extension, ".hlsl") || HasExtension(extension, ".fx") || HasExtension(extension, ".shader"))
			{
				return "";
			}

			if (HasExtension(extension, ".cs") ||
				HasExtension(extension, ".cpp") ||
				HasExtension(extension, ".h") ||
				HasExtension(extension, ".hpp"))
			{
				return "";
			}

			if (HasExtension(extension, ".wav") || HasExtension(extension, ".mp3") || HasExtension(extension, ".ogg"))
			{
				return "";
			}

			if (HasExtension(extension, ".ttf") || HasExtension(extension, ".otf"))
			{
				return "";
			}

			if (HasExtension(extension, ".md") ||
				HasExtension(extension, ".txt") ||
				HasExtension(extension, ".json") ||
				HasExtension(extension, ".xml") ||
				HasExtension(extension, ".yaml") ||
				HasExtension(extension, ".yml"))
			{
				return "";
			}

			return "";
		}

		private static bool HasExtension(string extension, string expected)
		{
			return string.Equals(extension, expected, StringComparison.OrdinalIgnoreCase);
		}
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
		public bool IsFolder { get; init; }
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
		public bool IsFolder => Asset == null || Asset.IsFolder;
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
