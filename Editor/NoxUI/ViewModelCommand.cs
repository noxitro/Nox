using System;
using System.Collections.Generic;
using System.Text;

namespace NoxUI
{
	public class ViewModelCommand : Prism.Commands.DelegateCommand
	{
		public ViewModelCommand(Action executeMethod) : base(executeMethod) { }

		public ViewModelCommand(Action executeMethod, Func<bool> canExecuteMethod) :
			base(executeMethod, canExecuteMethod)
		{ }
	}

	internal class ViewModelCommandHandler
	{
		#region 公開メソッド
		public ViewModelCommand Get(Action executeMethod)
		{
			if (_Command == null)
			{
				_Command = new ViewModelCommand(executeMethod);
			}

			return _Command;
		}

		public ViewModelCommand Get(Action executeMethod, Func<bool> canExecuteMethod)
		{
			if (_Command == null)
			{
				_Command = new ViewModelCommand(executeMethod, canExecuteMethod);
			}

			return _Command;
		}
		#endregion

		#region 非公開フィールド
		private ViewModelCommand? _Command = null;
		#endregion
	}

}
