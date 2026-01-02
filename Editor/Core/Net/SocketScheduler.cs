using System;
using System.Collections.Generic;
using System.Threading;

namespace Core.Net
{
	public class SocketScheduler : Nox.ISingleton<SocketScheduler>
	{
		#region 非公開フィールド
		private readonly List<Core.Net.Client> _ClientList = new();
		private readonly Nox.LockObject _LockClientList = new();

		private bool _IsStopped;
		#endregion

		#region 公開プロパティ
		public static SocketScheduler Instance => Nox.ISingleton<SocketScheduler>.Instance;
		public static bool HasInstance => Nox.ISingleton<SocketScheduler>.HasInstance;
		#endregion

		#region 公開メソッド
		public static void CreateInstance()
		{
			Nox.ISingleton<SocketScheduler>.CreateInstance();
		}

		public static void DeleteInstance()
		{
			if (HasInstance)
			{
			}

			Nox.ISingleton<SocketScheduler>.DeleteInstance();
		}

		public SocketScheduler()
		{
			var task = Task.Run(Update);
		}

		public void RegisterClient(Core.Net.Client client)
		{
			lock(_LockClientList)
			{
				_ClientList.Add(client);
			}
		}

		public void UnregisterClient(Core.Net.Client client)
		{
			lock(_LockClientList)
			{
				_ClientList.Remove(client);
			}
		}
		#endregion

		#region 非公開メソッド
		private void RunLoop(CancellationToken token)
		{
			const int intervalMs = 10;

			while (!token.IsCancellationRequested)
			{
				Update();

				if (token.WaitHandle.WaitOne(intervalMs))
				{
					break;
				}
			}
		}


		private void Update()
		{
			while (true)
			{
				lock (_LockClientList)
				{
					foreach (var client in _ClientList)
					{
						// クライアント更新処理をここに追加
					}
				}
			}
		}
		#endregion
	}
}
