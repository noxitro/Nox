using Nox;
using System;
using System.Diagnostics;
using System.IO;

namespace Core.UI.ViewModels
{
	/// <summary>
	/// RuntimeWindowを表示するためのViewModel
	/// </summary>
	public class RuntimeViewModel : ToolViewModel
	{
		#region 公開プロパティ
		public Core.RuntimeWrapper.SceneView? MainView { get; set; } = null;
		#endregion

		public RuntimeViewModel()
		{
//			Title = "Runtime";
//			Core.Runtime.Instance.ProcessChanged += OnRuntimeProcessChanged;
		}

		private void OnRuntimeProcessChanged(object? sender, EventArgs e)
		{
		}
	}
}