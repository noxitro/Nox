using System;
using System.Collections.Generic;
using System.Text;

namespace Core
{
	public sealed class EntryManager : Nox.ISingleton<EntryManager>
	{
		public enum Category : byte
		{
			Init,
			Finalize,
		}

		public static EntryManager Instance => Nox.ISingleton<EntryManager>.Instance;

		public static void CreateInstance()
		{
			Nox.ISingleton<EntryManager>.CreateInstance();
		}

		public static void DeleteInstance()
		{
			Nox.ISingleton<EntryManager>.DeleteInstance();
		}

		public void Register(Action action, EntryBase.EntryOrder order)
		{
			_ActTable[(int)order] = action;
		}

		public void InvokeStart()
		{
			for (uint i= (uint)EntryBase.EntryOrder._Init + 1; i < (uint)EntryBase.EntryOrder._Finalize; ++i)
			{
				_ActTable[i]?.Invoke();
			}
		}

		public void InvokeFinalize()
		{
			for (uint i = (uint)EntryBase.EntryOrder._Finalize + 1; i < (uint)EntryBase.EntryOrder._Max; ++i)
			{
				_ActTable[i]?.Invoke();
			}
		}

		private Action[] _ActTable = new Action[(int)EntryBase.EntryOrder._Max];
	}


	file class Entry : EntryBase
	{
		public Entry()
		{
			Register(Dispatch, EntryOrder.InitCore);
			Register(Dispose, EntryOrder.FinalizeCore);
		}

		private void Dispatch()
		{
			Core.StudioManager.CreateInstance();
			Core.Net.SocketScheduler.CreateInstance();
			Core.Runtime.CreateInstance();
		}

		public void Dispose()
		{
			Core.Runtime.DeleteInstance();
			Core.Net.SocketScheduler.DeleteInstance();
			Core.StudioManager.DeleteInstance();
		}
	}
}
