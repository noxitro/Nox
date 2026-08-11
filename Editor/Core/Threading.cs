using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Threading.Tasks;

	public static class Task
	{
		/// <summary>
		/// 例外処理付き Task.Run
		/// </summary>
		/// <param name="action"></param>
		/// <returns></returns>
		public static System.Threading.Tasks.Task Run(Action action)
		{
			Action wrapAction = () =>
			{
				try
				{
					action();
				}
				catch (Exception ex)
				{
					//Core.UI.MessageBox.Show(ex.ToString(), "例外が発生しました", MessageBoxLevel.Error);
					Nox.Util.Assert(false, "例外が発生しました: {0}", ex.ToString());
					throw;
				}
			};

			return System.Threading.Tasks.Task.Run(wrapAction);
		}
	}
