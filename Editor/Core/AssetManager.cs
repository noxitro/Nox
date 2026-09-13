// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

namespace Core;

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

	public string Rename(ProjectAsset asset, string requestedName)
	{
		ArgumentNullException.ThrowIfNull(asset);

		string trimmedName = requestedName.Trim();
		ValidateRename(trimmedName);
		if (string.Equals(trimmedName, asset.Name, StringComparison.Ordinal))
		{
			return asset.RelativePath;
		}

		string sourcePath = asset.FullPath;
		string parentDirectory = Path.GetDirectoryName(sourcePath)
			?? throw new InvalidOperationException($"Asset path is invalid: {sourcePath}");
		string targetPath = Path.Combine(parentDirectory, trimmedName);
		if (string.Equals(sourcePath, targetPath, StringComparison.OrdinalIgnoreCase))
		{
			return asset.RelativePath;
		}

		EnsureRenameTargetDoesNotExist(targetPath);
		if (asset.IsFolder)
		{
			Directory.Move(sourcePath, targetPath);
		}
		else
		{
			File.Move(sourcePath, targetPath);
		}

		MoveMetaIfNeeded(GetMetaPath(sourcePath), GetMetaPath(targetPath));
		return Path.GetRelativePath(_Workspace.AssetRootPath, targetPath);
	}

	public string CreateFolder(string parentRelativePath, string baseName)
	{
		string parentPath = string.IsNullOrWhiteSpace(parentRelativePath)
			? _Workspace.AssetRootPath
			: Path.Combine(_Workspace.AssetRootPath, parentRelativePath);
		Directory.CreateDirectory(parentPath);

		string folderName = GetUniqueName(parentPath, baseName, extension: null, separator: " ");
		string folderPath = Path.Combine(parentPath, folderName);
		Directory.CreateDirectory(folderPath);
		return Path.GetRelativePath(_Workspace.AssetRootPath, folderPath);
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
			AssetMeta meta = EnsureMeta(childDirectoryPath, relativePath, string.Empty, isFolder: true);
			ProjectAsset folderAsset = new()
			{
				Guid = meta.Guid,
				Name = directoryName,
				FullPath = childDirectoryPath,
				MetaPath = GetMetaPath(childDirectoryPath),
				RelativePath = relativePath,
				Extension = string.Empty,
				IsFolder = true,
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
			string extension = fileInfo.Extension;
			AssetMeta meta = EnsureMeta(filePath, relativePath, extension);
			ProjectAsset asset = new()
			{
				Guid = meta.Guid,
				Name = Path.GetFileName(filePath),
				FullPath = filePath,
				MetaPath = GetMetaPath(filePath),
				RelativePath = relativePath,
				Extension = extension,
				Size = fileInfo.Length,
				LastWriteTime = fileInfo.LastWriteTime,
				Uri = CreateAssetUri(relativePath),
			};

			_AssetList.Add(asset);
			parentNode.AddChild(new AssetTreeNode(asset.Name, relativePath, asset));
		}
	}

	private AssetMeta EnsureMeta(string assetPath, string relativePath, string extension, bool isFolder = false)
	{
		string metaPath = GetMetaPath(assetPath);
		AssetMeta? meta = ReadMeta(metaPath);
		if (meta == null)
		{
			meta = new AssetMeta
			{
				AssetPath = NormalizeAssetPath(relativePath),
				Importer = AssetTypeUtility.GetImporterName(extension, isFolder),
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

			string importer = AssetTypeUtility.GetImporterName(extension, isFolder);
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
		return new JsonSerializerOptions
		{
			WriteIndented = true,
		};
	}

	private static bool IsMetaFile(FileInfo fileInfo)
	{
		return fileInfo.Extension.Equals(".meta", StringComparison.OrdinalIgnoreCase);
	}

	private static string GetMetaPath(string assetPath)
	{
		return assetPath + ".meta";
	}

	private static void EnsureRenameTargetDoesNotExist(string targetPath)
	{
		if (File.Exists(targetPath) || Directory.Exists(targetPath))
		{
			throw new IOException($"An asset with the same name already exists: {targetPath}");
		}

		string targetMetaPath = GetMetaPath(targetPath);
		if (File.Exists(targetMetaPath) || Directory.Exists(targetMetaPath))
		{
			throw new IOException($"A meta file with the same name already exists: {targetMetaPath}");
		}
	}

	private static void MoveMetaIfNeeded(string sourceMetaPath, string targetMetaPath)
	{
		if (File.Exists(sourceMetaPath) == false)
		{
			return;
		}

		File.Move(sourceMetaPath, targetMetaPath);
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

	private static void ValidateRename(string name)
	{
		if (string.IsNullOrWhiteSpace(name))
		{
			throw new ArgumentException("Asset name cannot be empty.", nameof(name));
		}

		if (Path.GetFileName(name) != name)
		{
			throw new ArgumentException("Asset name cannot contain path separators.", nameof(name));
		}

		if (name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0)
		{
			throw new ArgumentException("Asset name contains invalid characters.", nameof(name));
		}
	}

	private static string GetUniqueName(string directoryPath, string baseName, string? extension, string separator)
	{
		string candidate = baseName;
		int suffix = 1;
		while (PathExists(directoryPath, candidate, extension))
		{
			candidate = $"{baseName}{separator}{suffix.ToString(System.Globalization.CultureInfo.InvariantCulture)}";
			++suffix;
		}

		return candidate;
	}

	private static bool PathExists(string directoryPath, string fileName, string? extension)
	{
		string fullPath = extension == null
			? Path.Combine(directoryPath, fileName)
			: Path.Combine(directoryPath, fileName + extension);
		return File.Exists(fullPath) || Directory.Exists(fullPath);
	}
}
