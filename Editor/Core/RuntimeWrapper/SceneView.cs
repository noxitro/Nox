using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::SceneView")]
	public class SceneView : ManagedObject
	{
		#region 公開プロパティ
		public event EventHandler? WindowHandleChanged;

		public SceneNode? Scene
		{
			get => Get<SceneNode>();
			set => Set(value);
		}

		public IntPtr WindowHandle
		{
			get;
			set
			{
				if (field == value)
				{
					return;
				}

				field = value;
				UpdateWindowHandle();
			}
		}
		#endregion

		#region 公開メソッド
		public SceneView()
		{

		}
		#endregion

		#region 非公開メソッド
		private void UpdateWindowHandle()
		{
			WindowHandleChanged?.Invoke(this, EventArgs.Empty);
		}
		#endregion
	}
}
