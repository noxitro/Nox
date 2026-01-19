using System;
using System.Collections.Generic;
using System.Text;

namespace Core
{
	namespace Attributes
	{
		/// <summary>
		/// メタ情報
		/// </summary>
		public sealed class MetaAttribute : System.Attribute
		{
		}
	}

	public abstract class Asset
	{
		#region 公開プロパティ
		[Core.Attributes.Meta]
		public System.Uri Uri { get; set; } = new System.Uri("assets:/");
		#endregion
	}
}
