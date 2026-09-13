// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Threading;

namespace Core.Net;

public class SocketScheduler : Core.EngineSystem
	{
		public static readonly Core.SystemPhaseTerminate<SocketScheduler> TerminatePhase = new(nameof(Stop), static engineSystem => engineSystem.Stop());

		#region 非公開フィールド
		private readonly List<Core.Net.Client> _ClientList = new();
		private readonly List<(Core.Net.Client Client, bool isAdd)> _ReqClientList = new();
		private readonly Nox.LockObject _LockClientList = new();

		private bool _IsStopped = false;
		#endregion

		#region 公開プロパティ
		public long UpdateCount { get; private set; }
		public int ClientCount => _ClientList.Count;
		public int PendingClientRequestCount => _ReqClientList.Count;
		public string LastError { get; private set; } = string.Empty;
		#endregion

		#region 公開メソッド
		public SocketScheduler()
		{
			var task = Nox.Threading.Tasks.Task.Run(Update);
		}

		public override Core.PhaseRegister[] GetPhaseRegisterList()
		{
			return
			[
				Core.PhaseRegister.Create(TerminatePhase, this),
			];
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
				try
				{
					UpdateCount++;
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
				}
				catch (Exception ex)
				{
					LastError = $"{ex.GetType().Name}: {ex.Message}";
					Nox.LogTrace.ErrorLine<Core.LogId.Net>("SocketScheduler update failed: {0}", ex);
				}

				Thread.Sleep(1);
			}
		}

		private void Stop()
		{
			_IsStopped = true;
		}

		#endregion
	}
