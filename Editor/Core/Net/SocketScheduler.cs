using System;
using System.Collections.Generic;
using System.Threading;

namespace Core.Net
{
	public class SocketScheduler : Nox.ISingleton<SocketScheduler>
	{
		#region 非公開フィールド
		private readonly List<Core.Net.Client> _ClientList = new();
		private readonly List<(Core.Net.Client Client, bool isAdd)> _ReqClientList = new();
		private readonly Nox.LockObject _LockClientList = new();

		private bool _IsStopped = false;
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
			var task = Nox.Threading.Tasks.Task.Run(Update);
		}

		public void RegisterClient(Core.Net.Client client)
		{
			lock(_LockClientList)
			{
				_ReqClientList.Add((client, true));
			}
		}

		public void UnregisterClient(Core.Net.Client client)
		{
			lock(_LockClientList)
			{
				_ReqClientList.Add((client, false));
			}
		}
		#endregion

		#region 非公開メソッド

		private void Update()
		{
			while (_IsStopped == false)
			{
				if (_ReqClientList.Count > 0)
				{
					lock (_LockClientList)
					{
						foreach (var req in _ReqClientList)
						{
							if (req.isAdd)
							{
								_ClientList.Add(req.Client);
							}
							else
							{
								_ClientList.Remove(req.Client);
							}
						}

						_ReqClientList.Clear();
					}
				}

				foreach(Client client in _ClientList)
				{
					client.Connection();
					client.Update();
				}

				Thread.Sleep(1);
			}
		}
		#endregion
	}
}
