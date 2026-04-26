using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::SceneNode")]
	public class SceneNode : ManagedObject
	{
		#region 非公開メソッド
		private List<EntityNode> _GameObjectList = new();
		#endregion

		#region 非公開メソッド
		#endregion

		#region 公開メソッド
		public SceneNode() 
		{

		}
		#endregion
	}
}
