using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::GameObject")]
	public class GameObject : Core.RuntimeObject, Core.IRuntimeObject<GameObject>
	{
		#region 非公開フィールド
		#endregion

		#region 公開メソッド
		public GameObject() : base(Core.IRuntimeObject<GameObject>.RuntimeRecordDecl)
		{

		}
		#endregion
	}
}
