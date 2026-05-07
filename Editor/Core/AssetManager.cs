using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Core
{
    public sealed class AssetManager : EngineSystem
	{
     #region 公開フィールド
		public static readonly SystemPhaseInit<AssetManager> InitPhase = new(nameof(Refresh), static engineSystem => engineSystem.Refresh());

        public static readonly SystemPhaseTerminate<AssetManager> TerminatePhase = new(nameof(Clear), static engineSystem => engineSystem.Clear());
		
		#endregion

		#region 非公開フィールド
		private static readonly HashSet<string> ExcludedDirectoryNames = new(StringComparer.OrdinalIgnoreCase)
		{
			".git",
			".vs",
			"bin",
			"obj",
			"build",
			"reflection_generated",
		};

		private readonly Workspace _Workspace;
		private readonly List<ProjectAsset> _AssetList = new();
		private readonly Dictionary<string, AssetMeta> _MetaByRelativePath = new(StringComparer.OrdinalIgnoreCase);
		private AssetTreeNode _RootNode;
		private EventHandler? _Changed;
		#endregion

		#region 公開プロパティ
		public IReadOnlyList<ProjectAsset> Assets => _AssetList;
		public AssetTreeNode RootNode => _RootNode;
		public string AssetRootPath => _Workspace.AssetRootPath;
		public event EventHandler? Changed
		{
			add => _Changed += value;
			remove => _Changed -= value;
		}
		#endregion

		public AssetManager(Workspace workspace)
		{
			_Workspace = workspace;
			_RootNode = CreateRootNode();
		}

		public override PhaseRegister[] GetPhaseRegisterList()
		{
			return
			[
				PhaseRegister.Create(InitPhase, this),
				PhaseRegister.Create(TerminatePhase, this),
			];
		}

		public void Refresh()
		{
			_AssetList.Clear();
			_MetaByRelativePath.Clear();
			_RootNode = CreateRootNode();

			if (Directory.Exists(_Workspace.ProjectPath) == false)
			{
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"Project path does not exist: {_Workspace.ProjectPath}");
				_Changed?.Invoke(this, EventArgs.Empty);
				return;
			}

			_Workspace.EnsureAssetRootDirectory();
			ScanDirectory(_Workspace.AssetRootPath, _RootNode);
			_AssetList.Sort(static (lhs, rhs) => string.Compare(lhs.RelativePath, rhs.RelativePath, StringComparison.OrdinalIgnoreCase));
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public void Clear()
		{
			_AssetList.Clear();
			_MetaByRelativePath.Clear();
			_RootNode = CreateRootNode();
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public IEnumerable<ProjectAsset> Search(string keyword)
		{
			if (string.IsNullOrWhiteSpace(keyword))
			{
				return _AssetList;
			}

			return _AssetList.Where(asset =>
				asset.Name.Contains(keyword, StringComparison.OrdinalIgnoreCase) ||
				asset.RelativePath.Contains(keyword, StringComparison.OrdinalIgnoreCase));
		}

		private AssetTreeNode CreateRootNode()
		{
			return new AssetTreeNode(_Workspace.AssetFolderName, string.Empty, null);
		}

		private void ScanDirectory(string directoryPath, AssetTreeNode parentNode)
		{
			IEnumerable<string> directories;
			IEnumerable<string> files;
			try
			{
				directories = Directory.EnumerateDirectories(directoryPath).OrderBy(static path => path, StringComparer.OrdinalIgnoreCase).ToArray();
				files = Directory.EnumerateFiles(directoryPath).OrderBy(static path => path, StringComparer.OrdinalIgnoreCase).ToArray();
			}
			catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
			{
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"Asset scan skipped: {directoryPath}, {ex}");
				return;
			}

			foreach (string childDirectoryPath in directories)
			{
				string directoryName = Path.GetFileName(childDirectoryPath);
				if (ExcludedDirectoryNames.Contains(directoryName))
				{
					continue;
				}

				string relativePath = Path.GetRelativePath(_Workspace.AssetRootPath, childDirectoryPath);
				AssetMeta meta = EnsureMeta(childDirectoryPath, relativePath, AssetKind.Folder);
				ProjectAsset folderAsset = new()
				{
					Guid = meta.Guid,
					Name = directoryName,
					FullPath = childDirectoryPath,
					MetaPath = GetMetaPath(childDirectoryPath),
					RelativePath = relativePath,
					Extension = string.Empty,
					Kind = AssetKind.Folder,
					LastWriteTime = Directory.GetLastWriteTime(childDirectoryPath),
					Uri = CreateAssetUri(relativePath),
				};
				AssetTreeNode childNode = parentNode.AddChild(new AssetTreeNode(directoryName, relativePath, folderAsset));
				ScanDirectory(childDirectoryPath, childNode);
			}

			foreach (string filePath in files)
			{
				FileInfo fileInfo = new(filePath);
				if (IsMetaFile(fileInfo))
				{
					continue;
				}

				string relativePath = Path.GetRelativePath(_Workspace.AssetRootPath, filePath);
				AssetKind kind = GetAssetKind(fileInfo.Extension);
				AssetMeta meta = EnsureMeta(filePath, relativePath, kind);
				ProjectAsset asset = new()
				{
					Guid = meta.Guid,
					Name = Path.GetFileName(filePath),
					FullPath = filePath,
					MetaPath = GetMetaPath(filePath),
					RelativePath = relativePath,
					Extension = fileInfo.Extension,
					Kind = kind,
					Size = fileInfo.Length,
					LastWriteTime = fileInfo.LastWriteTime,
					Uri = CreateAssetUri(relativePath),
				};

				_AssetList.Add(asset);
				parentNode.AddChild(new AssetTreeNode(asset.Name, relativePath, asset));
			}
		}

		private AssetMeta EnsureMeta(string assetPath, string relativePath, AssetKind kind)
		{
			string metaPath = GetMetaPath(assetPath);
			AssetMeta? meta = ReadMeta(metaPath);
			if (meta == null)
			{
				meta = new AssetMeta
				{
					AssetPath = NormalizeAssetPath(relativePath),
					Kind = kind,
					Importer = GetImporterName(kind),
				};
				WriteMeta(metaPath, meta);
			}
			else
			{
				bool changed = false;
				string normalizedPath = NormalizeAssetPath(relativePath);
				if (meta.AssetPath != normalizedPath)
				{
					meta.AssetPath = normalizedPath;
					changed = true;
				}

				if (meta.Kind != kind)
				{
					meta.Kind = kind;
					changed = true;
				}

				string importer = GetImporterName(kind);
				if (meta.Importer != importer)
				{
					meta.Importer = importer;
					changed = true;
				}

				if (string.IsNullOrWhiteSpace(meta.Guid))
				{
					meta.Guid = Guid.NewGuid().ToString("N");
					changed = true;
				}

				if (changed)
				{
					meta.UpdatedAtUtc = DateTime.UtcNow;
					WriteMeta(metaPath, meta);
				}
			}

			_MetaByRelativePath[relativePath] = meta;
			return meta;
		}

		private static AssetMeta? ReadMeta(string metaPath)
		{
			if (File.Exists(metaPath) == false)
			{
				return null;
			}

			try
			{
				return JsonSerializer.Deserialize<AssetMeta>(File.ReadAllText(metaPath), CreateMetaJsonOptions());
			}
			catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException)
			{
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"Asset meta read failed: {metaPath}, {ex}");
				return null;
			}
		}

		private static void WriteMeta(string metaPath, AssetMeta meta)
		{
			Directory.CreateDirectory(Path.GetDirectoryName(metaPath) ?? string.Empty);
			File.WriteAllText(metaPath, JsonSerializer.Serialize(meta, CreateMetaJsonOptions()));
		}

		private static JsonSerializerOptions CreateMetaJsonOptions()
		{
			JsonSerializerOptions options = new()
			{
				WriteIndented = true,
			};
			options.Converters.Add(new JsonStringEnumConverter());
			return options;
		}

		private static bool IsMetaFile(FileInfo fileInfo)
		{
			return fileInfo.Extension.Equals(".meta", StringComparison.OrdinalIgnoreCase);
		}

		private static string GetMetaPath(string assetPath)
		{
			return assetPath + ".meta";
		}

		private static AssetKind GetAssetKind(string extension)
		{
			return extension.ToLowerInvariant() switch
			{
				".noxscene" or ".scene" => AssetKind.Scene,
				".fbx" or ".obj" or ".gltf" or ".glb" => AssetKind.Model,
				".png" or ".jpg" or ".jpeg" or ".tga" or ".bmp" or ".dds" => AssetKind.Texture,
				".mat" or ".material" => AssetKind.Material,
				".hlsl" or ".fx" or ".shader" => AssetKind.Shader,
				".cs" or ".cpp" or ".h" or ".hpp" => AssetKind.Script,
				".wav" or ".mp3" or ".ogg" => AssetKind.Audio,
				".ttf" or ".otf" => AssetKind.Font,
				".md" or ".txt" or ".json" or ".xml" or ".yaml" or ".yml" => AssetKind.Document,
				_ => AssetKind.Unknown,
			};
		}

		private static Uri CreateAssetUri(string relativePath)
		{
			string normalizedPath = NormalizeAssetPath(relativePath);
			return new Uri($"assets:/{normalizedPath}", UriKind.Absolute);
		}

		private static string NormalizeAssetPath(string relativePath)
		{
			return relativePath.Replace(Path.DirectorySeparatorChar, '/').Replace(Path.AltDirectorySeparatorChar, '/');
		}

		private static string GetImporterName(AssetKind kind)
		{
			return kind switch
			{
				AssetKind.Folder => "FolderImporter",
				AssetKind.Scene => "SceneImporter",
				AssetKind.Model => "ModelImporter",
				AssetKind.Texture => "TextureImporter",
				AssetKind.Material => "MaterialImporter",
				AssetKind.Shader => "ShaderImporter",
				AssetKind.Script => "ScriptImporter",
				AssetKind.Audio => "AudioImporter",
				AssetKind.Font => "FontImporter",
				AssetKind.Document => "TextImporter",
				_ => "DefaultImporter",
			};
		}

	}
}
