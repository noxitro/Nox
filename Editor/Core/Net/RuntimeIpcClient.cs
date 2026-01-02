using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Net
{
	public class RuntimeIpcClient : Core.Net.Client, Nox.ISingleton<RuntimeIpcClient>, IDisposable
	{
		#region 公開プロパティ
		public static RuntimeIpcClient Instance => Nox.ISingleton<RuntimeIpcClient>.Instance;
		public static bool HasInstance => Nox.ISingleton<RuntimeIpcClient>.HasInstance;
		#endregion

		#region 公開メソッド
		public static void CreateInstance()
		{
			Nox.ISingleton<RuntimeIpcClient>.CreateInstance();
		}
		public static void DeleteInstance()
		{
			Nox.ISingleton<RuntimeIpcClient>.DeleteInstance();
		}

		public RuntimeIpcClient()
		{
			Core.Net.SocketScheduler.Instance.RegisterClient(this);
		}

		void IDisposable.Dispose()
		{
			Core.Net.SocketScheduler.Instance.UnregisterClient(this);
		}
		#endregion
	}
}
