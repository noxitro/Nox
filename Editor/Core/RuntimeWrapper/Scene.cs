using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::Scene")]
	public class Scene : ManagedObject, Core.IRuntimeObject<Scene>
	{
		#region 非公開メソッド
		public static new RuntimeRecordDecl StaticRuntimeRecordDecl { get; set; } = null!;
		private List<GameObject> _GameObjectList = new();
		#endregion

		#region 非公開メソッド
		protected override RuntimeRecordDecl GetRuntimeRecordDecl() => StaticRuntimeRecordDecl;
		#endregion

		#region 公開メソッド
		public Scene() 
		{

		}
		#endregion
	}
}
