using System;

namespace Core.UI.ViewModels
{
	/// <summary>
	/// RuntimeWindowを表示するためのViewModel
	/// </summary>
	public class RuntimeViewModel : ToolViewModel
	{
		#region 非公開フィールド
		private readonly Core.RuntimeSession _RuntimeSession;
		#endregion

		#region 公開プロパティ
		public Core.RuntimeWrapper.SceneView? MainView
		{
			get => field;
			set => SetProperty(ref field, value);
		}
		#endregion

		public RuntimeViewModel()
		{
			Title = "Runtime";
			_RuntimeSession = Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession();
			_RuntimeSession.ProcessChanged += OnRuntimeProcessChanged;
			_RuntimeSession.MainSceneViewChanged += OnMainSceneViewChanged;
			UpdateMainView();
		}

		private void OnRuntimeProcessChanged(object? sender, EventArgs e)
		{
			UpdateMainView();
		}

		private void OnMainSceneViewChanged(object? sender, EventArgs e)
		{
			UpdateMainView();
		}

		private void UpdateMainView()
		{
			System.Windows.Threading.Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (dispatcher != null && dispatcher.CheckAccess() == false)
			{
				dispatcher.BeginInvoke((Action)UpdateMainView);
				return;
			}

			MainView = _RuntimeSession.MainSceneView;
		}
	}
}
