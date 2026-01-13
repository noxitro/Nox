using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::Scene")]
	public class Scene : Core.RuntimeObject, Core.IRuntimeObject<Scene>
	{
		#region 非公開メソッド
		private List<GameObject> _GameObjectList = new();
		#endregion

		#region 公開メソッド
		public Scene() : base(Core.IRuntimeObject<Scene>.RuntimeRecordDecl)
		{

		}
		#endregion
	}
}
