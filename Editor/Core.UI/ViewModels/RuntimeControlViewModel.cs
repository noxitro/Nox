// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

namespace Core.UI.ViewModels;

	/// <summary>
	/// Runtime プロセスとの接続の状態。
	/// </summary>
	public enum RuntimeLinkState : byte
	{
		Disconnected,
		Connecting,
		Connected,
	}

	/// <summary>
	/// runtimeの再生ボタンなど。操作レールに出す Runtime プロセスと接続の状態も持つ。
	/// </summary>
	public class RuntimeControlViewModel : NoxUI.ViewModelBase
	{
		#region 非公開フィールド
		/// <summary>
		/// 状態の更新間隔。プロセスの終了と TCP の切断には通知が無いので、短い間隔で読む。
		/// 1 回の更新はフィールドを数個読むだけで、確保も通信も起こさない。
		/// </summary>
		private static readonly System.TimeSpan StateInterval = System.TimeSpan.FromMilliseconds(500);

		private readonly System.Windows.Threading.DispatcherTimer _StateTimer;
		private bool _IsProcessRunning;
		private RuntimeLinkState _LinkState;
		#endregion

		#region 公開プロパティ
		public NoxUI.ViewModelCommand RebootCommand => field ??= new(Reboot);
		public NoxUI.ViewModelCommand PlayCommand => field ??= new(Play);
		public NoxUI.ViewModelCommand StopCommand => field ??= new(Stop);
		public NoxUI.ViewModelCommand StartConnectionCommand => field ??= new(StartConnection);

		/// <summary>Runtime プロセスが起動しているか。</summary>
		public bool IsProcessRunning
		{
			get => _IsProcessRunning;
			private set
			{
				if (SetProperty(ref _IsProcessRunning, value))
				{
					RaisePropertyChanged(nameof(ProcessStateText));
				}
			}
		}

		/// <summary>Runtime プロセスの状態の短い名前。</summary>
		public string ProcessStateText => _IsProcessRunning ? "Running" : "Stopped";

		/// <summary>Runtime との TCP 接続の状態。</summary>
		public RuntimeLinkState LinkState
		{
			get => _LinkState;
			private set
			{
				if (SetProperty(ref _LinkState, value))
				{
					RaisePropertyChanged(nameof(LinkStateText));
				}
			}
		}

		/// <summary>接続の状態の短い名前。</summary>
		public string LinkStateText => _LinkState switch
		{
			RuntimeLinkState.Connected => "Connected",
			RuntimeLinkState.Connecting => "Connecting",
			_ => "Disconnected",
		};
		#endregion

		public RuntimeControlViewModel()
		{
			_StateTimer = new System.Windows.Threading.DispatcherTimer(System.Windows.Threading.DispatcherPriority.Background)
			{
				Interval = StateInterval,
			};
			_StateTimer.Tick += OnStateTimerTick;
			_StateTimer.Start();
			UpdateState();
		}

		public override void Dispose()
		{
			_StateTimer.Stop();
			_StateTimer.Tick -= OnStateTimerTick;
			base.Dispose();
		}

		#region 非公開メソッド
		private void OnStateTimerTick(object? sender, System.EventArgs e)
		{
			UpdateState();
		}

		private void UpdateState()
		{
			Core.RuntimeSession session = Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession();

			System.Diagnostics.Process? process = session.Process;
			bool running;
			try
			{
				running = process != null && process.HasExited == false;
			}
			catch (System.InvalidOperationException)
			{
				// プロセスに関連付いていない Process オブジェクト。起動していない扱いにする
				running = false;
			}
			IsProcessRunning = running;

			LinkState = session.RemoteClient.State switch
			{
				Core.Net.Client.ConnectionState.Connected => RuntimeLinkState.Connected,
				Core.Net.Client.ConnectionState.Disconnect => RuntimeLinkState.Disconnected,
				_ => RuntimeLinkState.Connecting,
			};
		}

		private void Reboot()
		{
			Core.RuntimeSession runtimeSession = Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession();
			if (runtimeSession.Reboot())
			{
				UpdateState();
				return;
			}

			Core.UI.MessageBox.ShowDialog(
				$"Runtime executable was not found.\n\nPath:\n{runtimeSession.RuntimeExecutablePath}",
				"Runtime launch failed",
				System.Windows.MessageBoxImage.Warning,
				System.Windows.MessageBoxButton.OK);
		}

		private void StartConnection()
		{
			Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession().StartTcpConnection();
			UpdateState();
		}

		private void Play()
		{
		}

		private void Stop()
		{

		}
		#endregion
	}
