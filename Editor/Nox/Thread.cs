using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Threading
{
	public class Thread
	{
	}

	namespace Tasks
	{
		public class Task
		{
			public static System.Threading.Tasks.Task Run(Action action)
			{
				try
				{
					return System.Threading.Tasks.Task.Run(action);
				}
				catch (Exception ex)
				{
					Console.WriteLine(ex.ToString());
					throw;
				}
			}
		}
	}
}
