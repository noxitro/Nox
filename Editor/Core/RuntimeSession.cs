using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using Nox;

namespace Core;

	public enum RuntimeSessionKind : byte
	{
		Main,
		ModelPreview,
		PlayClient,
		DedicatedServer,
		Custom,
	}

	public sealed class RuntimeSession : IDisposable
	{
		#region 非公開フィールド
		private static int _NextId;
		private Process? _Process;
		private EventHandler? _ProcessChanged;
		private EventHandler? _MainSceneViewChanged;
		private Nox.DelegateHandle _HandleRuntimeConnected = default;
		private readonly Core.Net.RuntimeRemoteClient _RemoteClient;
		//private Core.RuntimeWrapper.SceneView? _MainSceneView;
		private IntPtr _MainSceneViewWindowHandle;
		private bool _IsMainSceneViewQueryPending;
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public int Id { get; }
		public Workspace Workspace { get; }
		public RuntimeSessionKind Kind { get; }
		public PlatformType Platform { get; set; } = PlatformType.X64;
		public ConfigurationType ConfigurationType { get; set; } = ConfigurationType.Debug;
		public string RuntimeExecutablePath => Path.GetFullPath(Path.Combine(
			Workspace.RuntimeRootPath,
			"build",
			"runtime",
			Platform.GetName().ToString(),
			ConfigurationType.GetName().ToString(),
			"runtime.exe"));
		public Process? Process => _Process;
		public Core.Net.RuntimeRemoteClient RemoteClient => _RemoteClient;
	//	public Core.RuntimeWrapper.SceneView? MainSceneView => _MainSceneView;
		public IntPtr MainSceneViewWindowHandle => _MainSceneViewWindowHandle;
		public event EventHandler? ProcessChanged
		{
			add => _ProcessChanged += value;
			remove => _ProcessChanged -= value;
		}
		public event EventHandler? MainSceneViewChanged
		{
			add => _MainSceneViewChanged += value;
			remove => _MainSceneViewChanged -= value;
		}
		#endregion

		public RuntimeSession(Workspace workspace, RuntimeSessionKind kind)
		{
			Workspace = workspace;
			Kind = kind;
			Id = Interlocked.Increment(ref _NextId);
			_RemoteClient = new Core.Net.RuntimeRemoteClient(this);
		}

		public bool Reboot()
		{
			Nox.Util.Assert(_Disposed == false, "RuntimeSession is disposed.");

			try
			{
				StopProcess();

				string runtimeExeFullPath = RuntimeExecutablePath;
				if (System.IO.File.Exists(runtimeExeFullPath) == false)
				{
					Nox.LogTrace.WarningLine<Core.LogId.Runtime>("Runtime.exe が見つかりません: {0}", runtimeExeFullPath);
					return false;
				}

				KillExistingRuntimeProcess(runtimeExeFullPath);

				ProcessStartInfo psi = new()
				{
					FileName = runtimeExeFullPath,
					Arguments = "--studio",//	studio モードで起動することで、Runtime側でスタジオからの接続待ち受けが有効になる
                WorkingDirectory = Path.GetDirectoryName(runtimeExeFullPath) ?? Environment.CurrentDirectory,
					UseShellExecute = true,
				};

				_Process = System.Diagnostics.Process.Start(psi);
				Nox.Util.Assert(_Process != null, "Runtime.exeの起動に失敗しました:{0}", runtimeExeFullPath);
				Process runtimeProcess = _Process;

				if (Workspace.ProjectSettings.EditorCore().VSAttachWithStartup)
				{
					//Nox.Util.VisualStudioAttachToProcess(_Process.Id, runtimeExeFullPath);
				}

				try
				{
					runtimeProcess.WaitForInputIdle(5000);
				}
				catch (Exception ex) when (ex is InvalidOperationException || ex is System.ComponentModel.Win32Exception)
				{
					Nox.LogTrace.WarningLine<Core.LogId.Runtime>("Runtime.exeの入力待機に失敗しました: {0}", ex);
				}

				_ProcessChanged?.Invoke(this, EventArgs.Empty);

				StartTcpConnection();
				return true;
			}
			catch (Exception ex)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>("Reboot Task Error: {0}", ex);
				throw;
			}
		}

		public void StartTcpConnection()
		{
			Nox.Util.Assert(_Disposed == false, "RuntimeSession is disposed.");

			_HandleRuntimeConnected.Dispose();
			_HandleRuntimeConnected = RemoteClient.RegisterRuntimeConnectedEvent(RuntimeConnected);
			RemoteClient.Startup(new Net.Client.InitializeContext()
			{
				Hostname = "127.0.0.1",
				Port = 86
			});
		}

		public void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			_HandleRuntimeConnected.Dispose();
			StopProcess();
			((IDisposable)RemoteClient).Dispose();
			_Disposed = true;
		}

		private void RuntimeConnected()
		{
			Workspace.SceneHierarchy.SyncRuntimeObjects();

			if (_MainSceneViewWindowHandle != IntPtr.Zero || _IsMainSceneViewQueryPending)
			{
				return;
			}

			_IsMainSceneViewQueryPending = true;
			RemoteClient.SendQuery(new Core.RuntimeRemote.GetMainSceneView(),
				(Core.RuntimeRemote.Response respose) =>
				{
					_IsMainSceneViewQueryPending = false;
					RuntimeRemote.SceneViewInfo sceneViewInfo = Nox.Util.Cast<Core.RuntimeRemote.SceneViewInfo>(respose);
					IntPtr mainWindowHandle = (IntPtr)sceneViewInfo.MainWindowHandle;
					Nox.Util.Assert(mainWindowHandle != IntPtr.Zero, "SceneViewのウィンドウハンドル取得に失敗しました");
					SetMainSceneViewWindowHandle(mainWindowHandle);
				});
		}

		//private void SetMainSceneView(Core.RuntimeWrapper.SceneView? sceneView)
		//{
		//	_MainSceneView = sceneView;
		//	_MainSceneViewChanged?.Invoke(this, EventArgs.Empty);
		//}

		private void SetMainSceneViewWindowHandle(IntPtr windowHandle)
		{
			_MainSceneViewWindowHandle = windowHandle;
			if (windowHandle == IntPtr.Zero)
			{
				_IsMainSceneViewQueryPending = false;
			}
			_MainSceneViewChanged?.Invoke(this, EventArgs.Empty);
		}

		private void StopProcess()
		{
			Process? process = _Process;
			if (process == null)
			{
				return;
			}

			_Process = null;
			TryKill(process);
			TryWaitForExit(process, 5000);

			process.Dispose();
			//SetMainSceneView(null);
			SetMainSceneViewWindowHandle(IntPtr.Zero);
			_ProcessChanged?.Invoke(this, EventArgs.Empty);
		}

		private static void KillExistingRuntimeProcess(string runtimeExeFullPath)
		{
			foreach (Process p in Process.GetProcessesByName("runtime"))
			{
				try
				{
					string? path;
					try
					{
						path = p.MainModule?.FileName;
					}
					catch (Exception ex) when (ex is InvalidOperationException || ex is System.ComponentModel.Win32Exception || ex is NotSupportedException)
					{
						Nox.LogTrace.WarningLine<Core.LogId.Runtime>("既存 runtime.exe のパス取得に失敗しました: {0}", ex);
						path = null;
					}

					if (!string.IsNullOrEmpty(path) && string.Equals(path, runtimeExeFullPath, StringComparison.OrdinalIgnoreCase))
					{
						TryKill(p);
						TryWaitForExit(p, 2000);
					}
				}
				catch (Exception ex)
				{
					Nox.LogTrace.ErrorLine<Core.LogId.Runtime>("既存 runtime.exe の終了に失敗しました: {0}", ex);
				}
				finally
				{
					p.Dispose();
				}
			}
		}

		private static void TryKill(Process process)
		{
			try
			{
				process.Kill();
			}
			catch (Exception ex) when (ex is InvalidOperationException || ex is System.ComponentModel.Win32Exception || ex is NotSupportedException)
			{
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>("runtime.exe の終了要求に失敗しました: {0}", ex);
			}
		}

		private static void TryWaitForExit(Process process, int milliseconds)
		{
			try
			{
				process.WaitForExit(milliseconds);
			}
			catch (Exception ex) when (ex is InvalidOperationException || ex is System.ComponentModel.Win32Exception || ex is NotSupportedException)
			{
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>("runtime.exe の終了待機に失敗しました: {0}", ex);
			}
		}
	}
