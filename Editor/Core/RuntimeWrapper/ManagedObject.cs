using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
	[Core.Attributes.RuntimeWrapper("nox::ManagedObject")]
	public abstract class ManagedObject : Core.RuntimeObject, Core.IRuntimeObject<ManagedObject>
	{
		#region 非公開フィールド
		public static RuntimeRecordDecl StaticRuntimeRecordDecl { get; set; } = null!;
		#endregion

		#region 非公開プロパティ
		protected abstract override RuntimeRecordDecl GetRuntimeRecordDecl();
		#endregion

		#region 公開メソッド
		protected ManagedObject() 
		{
		}
		#endregion
	}
}
