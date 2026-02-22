using Core.RuntimeWrapper;
using Nox;
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;
using System.Runtime.InteropServices;
namespace Core
{
	public class Runtime : Nox.ISingleton<Runtime>, System.IDisposable
	{
		#region 非公開フィールド

		private System.Diagnostics.Process? _Process = default;
		private EventHandler? _ProcessChanged;
		private Nox.DelegateHandle _HandleRuntimeConnected=default;

		private Core.RuntimeWrapper.SceneView? MainSceneView = null;
		#endregion

		#region 公開プロパティ
		public static Runtime Instance => Nox.ISingleton<Runtime>.Instance;
		public PlatformType Platform { get; set; } = PlatformType.X64;
		public ConfigurationType ConfigurationType { get; set; } = ConfigurationType.Debug;
		public RuntimeTypeDB TypeDB { get; set; } = new RuntimeTypeDB();

		public System.Diagnostics.Process? Process => _Process;
		public event EventHandler? ProcessChanged
		{
			add => _ProcessChanged += value;
			remove => _ProcessChanged -= value;
		}
		#endregion

		#region 公開メソッド
		public static void CreateInstance()
		{
			Nox.ISingleton<Runtime>.CreateInstance();
		}
		public static void DeleteInstance()
		{
			((System.IDisposable)Instance).Dispose();
			Nox.ISingleton<Runtime>.DeleteInstance();
		}

		public Runtime()
		{
			Initialize();
		}

		void IDisposable.Dispose()
		{
			_HandleRuntimeConnected.Dispose();

			if (_Process != null)
			{
				_Process.Kill();
				_Process = null;
			}
		}

		public void Reboot()
		{
			try
			{
				if (_Process != null)
				{
					_Process.Kill();
					_Process = null;
					_ProcessChanged?.Invoke(this, EventArgs.Empty);
				}

				string runtimeExePath = $"runtime\\build\\runtime\\{Platform.GetName()}\\{ConfigurationType.GetName()}\\runtime.exe";
				string runtimeExeFullPath = System.IO.Path.GetFullPath(StudioManager.Instance.StudioInfo.ProjectPath + "\\" + runtimeExePath);

				Nox.Util.Assert(System.IO.File.Exists(runtimeExeFullPath) == true, "Runtime.exeが存在しません:{0}", runtimeExeFullPath);

				//	プロセスにruntime.exeが存在するか確認
				foreach (var p in System.Diagnostics.Process.GetProcessesByName("runtime"))
				{
					try
					{
						string? path;
						try
						{
							path = p.MainModule?.FileName;
						}
						catch
						{
							// アクセスできない場合もあるので握りつぶす
							path = null;
						}

						// パスが一致するものだけ殺す
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
				}

				System.Diagnostics.ProcessStartInfo psi = new()
				{
					FileName = runtimeExeFullPath,
					Arguments = "-Studio",
					WorkingDirectory = System.IO.Path.GetDirectoryName(runtimeExeFullPath) ?? Environment.CurrentDirectory,
					UseShellExecute = true,
				};

				_Process = System.Diagnostics.Process.Start(psi);
					
				Nox.Util.Assert(_Process != null, "Runtime.exeの起動に失敗しました:{0}", runtimeExeFullPath);

				// 子ウィンドウとしてドッキングできるよう、メインウィンドウが初期化されるまで待つ
				try { _Process.WaitForInputIdle(5000); } catch { /* 無視 */ }
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
			//	runtimeへ接続
			_HandleRuntimeConnected.Dispose();
			_HandleRuntimeConnected = Core.Net.RuntimeRemoteClient.Instance.RegisterRuntimeConnectedEvent(RuntimeConnected);
			Core.Net.RuntimeRemoteClient.Instance.Startup(new Net.Client.InitializeContext()
			{
				Hostname = "127.0.0.1",
				Port = 86
			});
		}

		public void PlayRuntime()
		{

		}
		#endregion

		#region 非公開メソッド
		private void Initialize()
		{
			BuildTypeDB();

			//	RuntimeWrapper型にDTIを設定する
			System.Type runtimeObjectType = typeof(RuntimeObject);
			//System.Type runtimeObjectInterfaceType = typeof(IRuntimeObject<>);
			//string runtimeObjectInterfaceTypeFullName = runtimeObjectInterfaceType.FullName ?? string.Empty;

			string propName = nameof(IRuntimeObject<>.StaticRuntimeRecordDecl);

			foreach (System.Type type in Core.TypeDB.AllTypeList)
			{
				if (runtimeObjectType.IsAssignableFrom(type) == false)
				{
					continue;
				}

				if (runtimeObjectType == type)
				{
					continue;
				}

				//Type? interfaceType = type.GetInterface(runtimeObjectInterfaceTypeFullName);
				//if (interfaceType == null)
				//{
				//	continue;
				//}

				var property = type.GetProperty(propName);
				if (property == null)
				{
					continue;
				}

				Core.Attributes.RuntimeWrapperAttribute? attr = type.GetCustomAttribute<Core.Attributes.RuntimeWrapperAttribute>();
				if (attr == null)
				{
					continue;
				}

				var runtimeType = TypeDB.FindType(attr.FQN);
				if (runtimeType == null)
				{
					Nox.LogTrace.ErrorLine<Core.LogId.Runtime>("RuntimeWrapperAttributeで指定された型がTypeDBに存在しません:{0}", attr.FQN);
					continue;
				}

				property.SetValue(null, (RuntimeRecordDecl)runtimeType.Decl);
			}
		}

		private void BuildTypeDB()
		{
			TypeDB = new RuntimeTypeDB();
			TypeDB.Build(Platform, ConfigurationType);
		}

		private void RuntimeConnected()
		{
			Core.Net.RuntimeRemoteClient.Instance.SendQuery(new Core.RuntimeRemote.GetMainSceneView(), 
				(Core.RuntimeRemote.Response respose) =>
				{
					//var sceneViewInfo = respose as Core.RuntimeRemote.SceneViewInfo;
					RuntimeRemote.SceneViewInfo sceneViewInfo = Nox.Util.Cast<Core.RuntimeRemote.SceneViewInfo>(respose);
					//Nox.Util.Assert(sceneViewInfo != null, "SceneViewInfoの取得に失敗しました");
					Nox.Util.Assert(sceneViewInfo.SceneView != null, "SceneViewの取得に失敗しました");

					MainSceneView = sceneViewInfo.SceneView;
					MainSceneView.WindowHandle = (System.IntPtr)sceneViewInfo.MainWindowHandle;
				}
				);
		}
		#endregion
	}
}
