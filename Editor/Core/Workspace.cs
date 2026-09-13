// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.IO;

namespace Core;

	public sealed class Workspace : IDisposable
	{
		#region 公開定数
		public const string DefaultAssetFolderName = "Assets";
		#endregion

		#region 非公開フィールド
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public string ProjectPath { get; }
		public ProjectSettings ProjectSettings { get; }
		public string AssetFolderName => string.IsNullOrWhiteSpace(ProjectSettings.AssetFolderName) ? DefaultAssetFolderName : ProjectSettings.AssetFolderName;
		public string AssetRootPath => GetFullPath(AssetFolderName);
		public string RuntimeRootPath => GetFullPath(ProjectSettings.RuntimeRootRelativePath);
		public AssetManager AssetManager { get; }
		public SceneHierarchyManager SceneHierarchy { get; }
		public SelectionService Selection { get; }
		public RuntimeSessionManager RuntimeSessions { get; }
		public LogService LogService { get; }
		#endregion

		public Workspace(string projectPath, ProjectSettings projectSettings)
		{
			Nox.Util.Assert(string.IsNullOrWhiteSpace(projectPath) == false, "ProjectPath is empty.");

			ProjectPath = Path.GetFullPath(projectPath);
			ProjectSettings = projectSettings;
			LogService = new LogService();
			LogService.Initialize();
			AssetManager = new AssetManager(this);
			SceneHierarchy = new SceneHierarchyManager();
			Selection = new SelectionService();
			RuntimeSessions = new RuntimeSessionManager(this);
		}

		public string GetFullPath(params string[] relativePaths)
		{
			string path = ProjectPath;
			foreach (string relativePath in relativePaths)
			{
				path = Path.Combine(path, relativePath);
			}

			return Path.GetFullPath(path);
		}

		public void EnsureAssetRootDirectory()
		{
			Directory.CreateDirectory(AssetRootPath);
		}

		public void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			RuntimeSessions.Dispose();
			((IDisposable)LogService).Dispose();
			_Disposed = true;
		}
	}
