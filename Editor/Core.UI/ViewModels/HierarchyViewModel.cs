using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.ViewModels
{
	public class HierarchyViewModel : NoxUI.ViewModelBase
	{
		#region 非公開フィールド

		#endregion

		#region 公開プロパティ
		private NoxUI.ViewModelCommand AddGameObjectCommand => field ??= new(AddGameObject);
		#endregion

		#region 非公開メソッド
		private void AddGameObject()
		{
			
		}
		#endregion
	}
}
