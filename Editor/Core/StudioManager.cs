using System;
using System.IO;

namespace Core
{
	public struct StudioInfo
	{
		public string ProjectPath { get; init; }
		public string RuntimeSolutionDir { get; init; }
	}

	public class StudioManager : Nox.ISingleton<StudioManager>
	{
		#region 非公開フィールド
		private StudioInfo _StudioInfo;
		private ProjectSettings _ProjectSettings = new();
		private Workspace _Workspace;
		#endregion

		#region 公開プロパティ
		public static StudioManager Instance => Nox.ISingleton<StudioManager>.Instance;
		public ref readonly StudioInfo StudioInfo => ref _StudioInfo;
		public ProjectSettings ProjectSettings => _ProjectSettings;
		public Workspace Workspace => _Workspace;
		#endregion

		#region 公開メソッド
		public static void CreateInstance()
		{
			Nox.ISingleton<StudioManager>.CreateInstance();
		}

		public static void DeleteInstance()
		{
			Instance._Workspace.Dispose();
			Nox.ISingleton<StudioManager>.DeleteInstance();
		}

		public StudioManager()
		{
			string projectPath = ResolveProjectPath();
			_Workspace = new Workspace(projectPath);

			_StudioInfo = new StudioInfo()
			{
				ProjectPath = _Workspace.ProjectPath,
				RuntimeSolutionDir = Path.Combine(_Workspace.ProjectPath, "runtime"),
			};
		}

		private static string ResolveProjectPath()
		{
			DirectoryInfo? directory = new(AppContext.BaseDirectory);
			while (directory != null)
			{
				string runtimeSolutionPath = Path.Combine(directory.FullName, "runtime", "runtime.sln");
				string editorPath = Path.Combine(directory.FullName, "Editor");
				if (File.Exists(runtimeSolutionPath) && Directory.Exists(editorPath))
				{
					return directory.FullName;
				}

				directory = directory.Parent;
			}

			DirectoryInfo baseDirectory = new(AppContext.BaseDirectory);
			Nox.Util.Assert(baseDirectory.Parent != null, "Parent directory is null");
			return baseDirectory.Parent!.FullName;
		}
		#endregion
	}
}
