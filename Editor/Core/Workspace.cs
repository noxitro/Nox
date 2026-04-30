using System;
using System.IO;

namespace Core
{
	public sealed class Workspace : IDisposable
	{
		#region 非公開フィールド
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public string ProjectPath { get; }
		public RuntimeSessionManager RuntimeSessions { get; }
		#endregion

		public Workspace(string projectPath)
		{
			Nox.Util.Assert(string.IsNullOrWhiteSpace(projectPath) == false, "ProjectPath is empty.");

			ProjectPath = Path.GetFullPath(projectPath);
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

		public void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			RuntimeSessions.Dispose();
			_Disposed = true;
		}
	}
}
