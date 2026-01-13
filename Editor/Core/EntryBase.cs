using System;
using System.Collections.Generic;
using System.Text;

namespace Core
{
	public abstract class EntryBase
	{
		protected void Register(Action action, EntryOrder order)
		{
			EntryManager.Instance.Register(action, order);
		}

		public enum EntryOrder : uint
		{
			_Init,
			InitCore,

			_Finalize,
			FinalizeCore,
			_Max
		}
	}
}
