using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using Nox;

namespace Core
{
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
		private Core.RuntimeWrapper.SceneView? _MainSceneView;
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public int Id { get; }
		public Workspace Workspace { get; }
		public RuntimeSessionKind Kind { get; }
		public PlatformType Platform { get; set; } = PlatformType.X64;
		public ConfigurationType ConfigurationType { get; set; } = ConfigurationType.Debug;
		public string RuntimeExecutablePath => Workspace.GetFullPath(
			"runtime",
			"build",
			"runtime",
			Platform.GetName().ToString(),
			ConfigurationType.GetName().ToString(),
			"runtime.exe");
		public Process? Process => _Process;
		public Core.Net.RuntimeRemoteClient RemoteClient => _RemoteClient;
		public Core.RuntimeWrapper.SceneView? MainSceneView => _MainSceneView;
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

		public void Reboot()
		{
			Nox.Util.Assert(_Disposed == false, "RuntimeSession is disposed.");

			try
			{
				StopProcess();

				string runtimeExeFullPath = RuntimeExecutablePath;
				Nox.Util.Assert(File.Exists(runtimeExeFullPath), "Runtime.exeが存在しません:{0}", runtimeExeFullPath);

				KillExistingRuntimeProcess(runtimeExeFullPath);

				ProcessStartInfo psi = new()
				{
					FileName = runtimeExeFullPath,
					Arguments = "-Studio",
					WorkingDirectory = Path.GetDirectoryName(runtimeExeFullPath) ?? Environment.CurrentDirectory,
					UseShellExecute = true,
				};

				_Process = System.Diagnostics.Process.Start(psi);
				Nox.Util.Assert(_Process != null, "Runtime.exeの起動に失敗しました:{0}", runtimeExeFullPath);

				try
				{
					_Process.WaitForInputIdle(5000);
				}
				catch (InvalidOperationException ex)
				{
					Nox.LogTrace.WarningLine<Core.LogId.Runtime>("Runtime.exeの入力待機に失敗しました: {0}", ex);
				}

				_ProcessChanged?.Invoke(this, EventArgs.Empty);

				StartTcpConnection();
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
			RemoteClient.SendQuery(new Core.RuntimeRemote.GetMainSceneView(),
				(Core.RuntimeRemote.Response respose) =>
				{
					RuntimeRemote.SceneViewInfo sceneViewInfo = Nox.Util.Cast<Core.RuntimeRemote.SceneViewInfo>(respose);
					Nox.Util.Assert(sceneViewInfo.SceneView != null, "SceneViewの取得に失敗しました");

					sceneViewInfo.SceneView.WindowHandle = (IntPtr)sceneViewInfo.MainWindowHandle;
					SetMainSceneView(sceneViewInfo.SceneView);
				});
		}

		private void SetMainSceneView(Core.RuntimeWrapper.SceneView? sceneView)
		{
			_MainSceneView = sceneView;
			_MainSceneViewChanged?.Invoke(this, EventArgs.Empty);
		}

		private void StopProcess()
		{
			if (_Process == null)
			{
				return;
			}

			_Process.Kill();
			_Process = null;
			SetMainSceneView(null);
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
						p.Kill();
						p.WaitForExit(2000);
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
	}
}
