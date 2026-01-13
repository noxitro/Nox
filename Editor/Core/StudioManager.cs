using System;
using System.Collections.Generic;
using System.Text;

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
		#endregion

		#region 公開プロパティ
		public static StudioManager Instance => Nox.ISingleton<StudioManager>.Instance;
		public ref readonly StudioInfo StudioInfo => ref _StudioInfo;
		public ProjectSettings ProjectSettings => _ProjectSettings;
		#endregion

		#region 公開メソッド
		public static void CreateInstance()
		{
			Nox.ISingleton<StudioManager>.CreateInstance();
		}

		public static void DeleteInstance()
		{
			Nox.ISingleton<StudioManager>.DeleteInstance();
		}

		public StudioManager()
		{
			_StudioInfo = new StudioInfo()
			{
				ProjectPath = "D:/github/Nox",
				RuntimeSolutionDir = "D:/github/Nox/runtime",
			};
		}
		#endregion
	}
}
