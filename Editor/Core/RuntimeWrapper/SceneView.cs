using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::SceneView")]
	public class SceneView : ManagedObject, Core.IRuntimeObject<SceneView>
	{
		#region 非公開フィールド
		public static new RuntimeRecordDecl StaticRuntimeRecordDecl { get; set; } = null!;
		#endregion

		#region 公開プロパティ
		public Scene? Scene
		{
			get => Get<Scene>();
			set => Set(value);
		}

		public IntPtr WindowHandle
		{
			get;
			set
			{
				field = value;
				UpdateWindowHandle(value);
			}
		}
		#endregion

		#region 公開メソッド
		public SceneView()
		{

		}
		#endregion

		#region 非公開メソッド
		protected override RuntimeRecordDecl GetRuntimeRecordDecl() => StaticRuntimeRecordDecl;
		private void UpdateWindowHandle(IntPtr windowHandle)
		{

		}
		#endregion
	}
}
