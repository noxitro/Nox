using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::SceneView")]
	internal class SceneView : Core.RuntimeObject, Core.IRuntimeObject<Scene>
	{
		#region 公開プロパティ
		public Scene? Scene { get; private set; }

		#endregion

		#region 公開メソッド
		public SceneView() : base(Core.IRuntimeObject<Scene>.RuntimeRecordDecl)
		{

		}
		#endregion

		#region 非公開メソッド

		#endregion
	}
}
