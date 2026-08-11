using System;

namespace Core.UI.ViewModels;

	/// <summary>
	/// RuntimeWindowを表示するためのViewModel
	/// </summary>
	public class RuntimeViewModel : ToolViewModel, IDisposable
	{
		#region 非公開フィールド
		private readonly Core.RuntimeSession _RuntimeSession;
		private readonly System.Windows.Threading.DispatcherTimer _StatusUpdateTimer;
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public IntPtr MainWindowHandle
		{
			get => field;
			set => SetProperty(ref field, value);
		}

		public string DebugStatus
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		#endregion

		public RuntimeViewModel()
		{
			Title = "Runtime";
			_RuntimeSession = Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession();
			_RuntimeSession.ProcessChanged += OnRuntimeProcessChanged;
			_RuntimeSession.MainSceneViewChanged += OnMainSceneViewChanged;
			_StatusUpdateTimer = new System.Windows.Threading.DispatcherTimer
			{
				Interval = TimeSpan.FromMilliseconds(250),
			};
			_StatusUpdateTimer.Tick += (_, _) => UpdateDebugStatus();
			_StatusUpdateTimer.Start();
			UpdateMainView();
			UpdateDebugStatus();
		}

		private void OnRuntimeProcessChanged(object? sender, EventArgs e)
		{
			if (_Disposed)
			{
				return;
			}

			UpdateMainView();
		}

		private void OnMainSceneViewChanged(object? sender, EventArgs e)
		{
			if (_Disposed)
			{
				return;
			}

			UpdateMainView();
		}

		private void UpdateMainView()
		{
			System.Windows.Threading.Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (NoxUI.DispatcherHelper.IsShuttingDown(dispatcher))
			{
				return;
			}

			if (dispatcher!.CheckAccess() == false)
			{
				NoxUI.DispatcherHelper.TryBeginInvoke(dispatcher, UpdateMainView);
				return;
			}

			//MainView = _RuntimeSession.MainSceneView;
			MainWindowHandle = _RuntimeSession.MainSceneViewWindowHandle;
			UpdateDebugStatus();
		}

		private void UpdateDebugStatus()
		{
			if (_Disposed)
			{
				return;
			}

			Core.Net.RuntimeRemoteClient remoteClient = _RuntimeSession.RemoteClient;
			Core.Net.SocketScheduler socketScheduler = Core.StudioManager.Instance.GetEngineSystem<Core.Net.SocketScheduler>();
			string handle = MainWindowHandle == IntPtr.Zero ? "null" : $"0x{MainWindowHandle.ToInt64():X}";
			DebugStatus = $"State={remoteClient.State}; Attempts={remoteClient.ConnectionAttemptCount}; Sent={remoteClient.SentQueryCount}; Received={remoteClient.ReceivedByteCount}; Deserialized={remoteClient.DeserializedEntityCount}; Handle={handle}; Error={remoteClient.LastWorkerError}; ConnectError={remoteClient.LastConnectionError}; Scheduler={socketScheduler.UpdateCount}; Clients={socketScheduler.ClientCount}; PendingClients={socketScheduler.PendingClientRequestCount}; SchedulerError={socketScheduler.LastError}";
		}

		public override void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			_RuntimeSession.ProcessChanged -= OnRuntimeProcessChanged;
			_RuntimeSession.MainSceneViewChanged -= OnMainSceneViewChanged;
			_StatusUpdateTimer.Stop();
			_Disposed = true;
		}
	}
