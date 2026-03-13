using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.ViewModels
{
	public class LogItemViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public string Message
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;
		#endregion
	}

	public class LogViewModel : Core.UI.ViewModels.DocumentViewModel
	{
		
	}
}
