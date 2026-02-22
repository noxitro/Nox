using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::GameObject")]
	public class GameObject : ManagedObject, Core.IRuntimeObject<GameObject>
	{
		#region 非公開フィールド
		public static new RuntimeRecordDecl StaticRuntimeRecordDecl { get; set; } = null!;
		#endregion

		#region 非公開メソッド
		protected override RuntimeRecordDecl GetRuntimeRecordDecl() => StaticRuntimeRecordDecl;
		#endregion

		#region 公開メソッド
		public GameObject()
		{

		}
		#endregion
	}
}
