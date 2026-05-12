using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Core
{
	public struct StudioInfo
	{
		public string ProjectPath { get; init; }
		public string RuntimeSolutionDir { get; init; }
	}

  public class StudioManager : Core.EngineSystem, Nox.ISingleton<StudioManager>
	{
		#region 非公開定数
		private const string ProjectPathEnvironmentVariableName = "NOX_STUDIO_PROJECT_PATH";
		#endregion

		#region 非公開フィールド
		private static string? _ConfiguredProjectPath;
		private StudioInfo _StudioInfo;
		private ProjectSettings _ProjectSettings = new();
		private Workspace _Workspace;
      private readonly List<EngineSystem> _EngineSystemList = new();
      private bool _Terminated;
		#endregion

		#region 公開プロパティ
		public ref readonly StudioInfo StudioInfo => ref _StudioInfo;
		public ProjectSettings ProjectSettings => _ProjectSettings;
		public Workspace Workspace => _Workspace;
      public static StudioManager Instance => Nox.ISingleton<StudioManager>.Instance;
		#endregion

		#region 公開メソッド

		public static void ConfigureProjectPath(string? projectPath)
		{
			_ConfiguredProjectPath = string.IsNullOrWhiteSpace(projectPath) ? null : Path.GetFullPath(projectPath);
		}

		public static void CreateInstance()
		{
			Nox.ISingleton<StudioManager>.CreateInstance();
		}

		public static void DeleteInstance()
		{
          Instance.InvokeTerminate();
			Nox.ISingleton<StudioManager>.DeleteInstance();
		}

		public StudioManager()
		{
			string projectPath = ResolveProjectPath();
			_ProjectSettings = ProjectSettings.LoadOrCreate(projectPath);
			_Workspace = new Workspace(projectPath, _ProjectSettings);

			_StudioInfo = new StudioInfo()
			{
				ProjectPath = _Workspace.ProjectPath,
				RuntimeSolutionDir = _Workspace.RuntimeRootPath,
			};

			RegisterEngineSystem(_Workspace.AssetManager);
			RegisterEngineSystem(_Workspace.SceneHierarchy);
		}

		public void RegisterEngineSystem(EngineSystem engineSystem)
		{
			_EngineSystemList.Add(engineSystem);
		}

		public T GetEngineSystem<T>() where T : EngineSystem
		{
			foreach (EngineSystem engineSystem in _EngineSystemList)
			{
				if (engineSystem is T typedEngineSystem)
				{
					return typedEngineSystem;
				}
			}

			throw new InvalidOperationException($"EngineSystem is not registered. Type={typeof(T).FullName}");
		}

		public IReadOnlyList<EngineSystem> GetEngineSystemList()
		{
			return _EngineSystemList;
		}

		public void InvokeInit()
		{
			Invoke(SystemPhaseType.Init);
		}

		public void InvokeStart()
		{
			Invoke(SystemPhaseType.Start);
		}

		public void InvokeTerminate()
		{
          if (_Terminated)
			{
				return;
			}

			_Terminated = true;
			Invoke(SystemPhaseType.Terminate);
			_Workspace.Dispose();
		}

		private void Invoke(SystemPhaseType phaseType)
		{
            List<PhaseRegister> phaseRegisters = _EngineSystemList
				.SelectMany(system => system.GetPhaseRegisterList())
				.Where(register => register.Phase.PhaseType == phaseType)
				.ToList();

			HashSet<string> executed = new(StringComparer.Ordinal);
			while (executed.Count < phaseRegisters.Count)
			{
				bool progressed = false;
				foreach (PhaseRegister register in phaseRegisters)
				{
					if (executed.Contains(register.Phase.Name))
					{
						continue;
					}

					if (CanExecute(register, phaseRegisters, executed) == false)
					{
						continue;
					}

					register.Proc();
					executed.Add(register.Phase.Name);
					progressed = true;
				}

				if (progressed == false)
				{
					throw new InvalidOperationException($"Circular or unresolved engine system dependency detected. PhaseType={phaseType}");
				}
			}
		}

		private static bool CanExecute(PhaseRegister register, IReadOnlyList<PhaseRegister> phaseRegisters, HashSet<string> executed)
		{
			if ((register.Dependencies ?? Array.Empty<SystemPhase>()).All(dependency => executed.Contains(dependency.Name)) == false)
			{
				return false;
			}

			foreach (PhaseRegister otherRegister in phaseRegisters)
			{
				if ((otherRegister.Dependents ?? Array.Empty<SystemPhase>()).Any(dependent => dependent.Name == register.Phase.Name) &&
					executed.Contains(otherRegister.Phase.Name) == false)
				{
					return false;
				}
			}

			return true;
		}

		private static string ResolveProjectPath()
		{
			if (string.IsNullOrWhiteSpace(_ConfiguredProjectPath) == false)
			{
				return _ConfiguredProjectPath;
			}

			string? environmentProjectPath = Environment.GetEnvironmentVariable(ProjectPathEnvironmentVariableName);
			if (string.IsNullOrWhiteSpace(environmentProjectPath) == false)
			{
				return Path.GetFullPath(environmentProjectPath);
			}

			DirectoryInfo? directory = new(AppContext.BaseDirectory);
			while (directory != null)
			{
				string runtimeSolutionPath = Path.Combine(directory.FullName, "runtime", "runtime.sln");
				string editorPath = Path.Combine(directory.FullName, "Editor");
				if (File.Exists(runtimeSolutionPath) && Directory.Exists(editorPath))
				{
					string sandboxPath = Path.Combine(directory.FullName, "sandbox");
					return Directory.Exists(sandboxPath) ? sandboxPath : directory.FullName;
				}

				directory = directory.Parent;
			}

			DirectoryInfo baseDirectory = new(AppContext.BaseDirectory);
			Nox.Util.Assert(baseDirectory.Parent != null, "Parent directory is null");
			return baseDirectory.Parent!.FullName;
		}
        #endregion

		#region 非公開メソッド
		public override PhaseRegister[] GetPhaseRegisterList()
		{
			return Array.Empty<PhaseRegister>();
		}
		#endregion
    }
}
