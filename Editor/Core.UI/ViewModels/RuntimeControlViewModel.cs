// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

namespace Core.UI.ViewModels;

	/// <summary>
	/// runtimeの再生ボタンなど
	/// </summary>
	public class RuntimeControlViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public NoxUI.ViewModelCommand RebootCommand => field ??= new(Reboot);
		public NoxUI.ViewModelCommand PlayCommand => field ??= new(Play);
		public NoxUI.ViewModelCommand StopCommand => field ??= new(Stop);
		public NoxUI.ViewModelCommand StartConnectionCommand => field ??= new(StartConnection);
		#endregion

		#region 非公開メソッド
		private void Reboot()
		{
			Core.RuntimeSession runtimeSession = Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession();
			if (runtimeSession.Reboot())
			{
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
		}

		private void Play()
		{
		}

		private void Stop()
		{

		}
		#endregion
	}
