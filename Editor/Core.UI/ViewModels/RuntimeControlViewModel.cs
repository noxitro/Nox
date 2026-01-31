using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.ViewModels
{
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
			Core.Runtime.Instance.Reboot();
		}

		private void StartConnection()
		{
			Core.Runtime.Instance.StartTcpConnection();
		}

		private void Play()
		{
		}

		private void Stop()
		{

		}
		#endregion
	}
}
