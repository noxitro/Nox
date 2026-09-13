// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Globalization;
using System.IO;
using System.Text;

namespace Core.Net;

	public class Client : Core.Net.Entity, IDisposable
	{
		#region 公開型定義
		public struct InitializeContext
		{
			public string Hostname;
			public ushort Port;

			public InitializeContext()
			{
				Hostname = "localhost";
				Port = 0;
			}
		}

		public struct PeerContext
		{
			public int Port;
			public uint UniqueId;
		}

		public enum ConnectionState : byte
		{
			Disconnect,
			Connection,
			HandShake1,
			HandShake2,
			HandShake3,
			Connected,
		}
		#endregion

		#region 非公開フィールド
		private const int ReceiveBufferSize = 4096;
		private const int HandshakeMaxLineLength = 512;

		private const int ConnectRetryDelayMs = 100;

		private const string HandShakeStr1 = "HandShake1";
		private const string HandShakeStr2 = "HandShake2";
		private const string HandShakeStr3 = "HandShake3";

		private PeerContext _PeerContext;
		private ConnectionState _ConnectionState = ConnectionState.Disconnect;
		private InitializeContext _InitContext = default;

		System.Net.IPEndPoint? _RemoteEndPoint = null;
		// 接続リトライ制御
		private DateTime _nextConnectAtUtc = DateTime.MinValue;
		private int _connectAttempts = 0;
		private readonly object _SocketSync = new();

		#endregion

		#region 公開プロパティ
		public bool IsConnected => _ConnectionState == ConnectionState.Connected && _Socket?.Connected == true;
		public ConnectionState State => _ConnectionState;
		public PeerContext Peer => _PeerContext;
		public string LastConnectionError { get; private set; } = string.Empty;
		public int ConnectionAttemptCount { get; private set; }
		#endregion

		#region 公開メソッド
		public void Startup(in InitializeContext context)
		{
			if (context.Port <= 0)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Net>("不正なPort:{0}", context.Port);
				throw new ArgumentOutOfRangeException(nameof(context.Port));
			}

			if (string.IsNullOrWhiteSpace(context.Hostname))
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Net>("hostnameが不正:{0}", context.Hostname);
				throw new ArgumentException("Hostname must be provided", nameof(context.Hostname));
			}

			Shutdown();

			// Dns は IPv6 を先に返すことがある。IPv4 ソケットで IPv6 アドレスに接続すると
			// "要求したプロトコルと互換性がないアドレス" になるため、接続先の AddressFamily に合わせてソケットを作る。
			var port = (int)context.Port;
			System.Net.IPAddress? address = null;
			try
			{
				var entry = System.Net.Dns.GetHostEntry(context.Hostname);
				foreach (var ip in entry.AddressList)
				{
					if (ip.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
					{
						address = ip;
						break;
					}
				}
				if (address is null)
				{
					foreach (var ip in entry.AddressList)
					{
						if (ip.AddressFamily == System.Net.Sockets.AddressFamily.InterNetworkV6)
						{
							address = ip;
							break;
						}
					}
				}
			}
			catch
			{
				// 例外は後段の Connect で扱う
			}

			address ??= System.Net.IPAddress.Loopback;
			lock (_SocketSync)
			{
				_InitContext = context;
				_RemoteEndPoint = new System.Net.IPEndPoint(address, port);
				_Socket = CreateSocket(address.AddressFamily);
				_ConnectionState = ConnectionState.Connection;
			}

			Core.StudioManager.Instance.GetEngineSystem<SocketScheduler>().RegisterClient(this);
		}

		void IDisposable.Dispose()
		{
			Shutdown();
		}

		public void Connection()
		{
			lock (_SocketSync)
			{
				if (_ConnectionState is ConnectionState.Connected or ConnectionState.Disconnect)
				{
					return;
				}

				Nox.Util.Assert(_RemoteEndPoint != null, "_LocalEndPoint is null in Connection state");
				_Socket ??= CreateSocket(_RemoteEndPoint.AddressFamily);

				switch (_ConnectionState)
				{
					case ConnectionState.Connection:
						// 次の試行時刻までは何もしない
						if (DateTime.UtcNow < _nextConnectAtUtc)
						{
							return;
						}
						ConnectionAttemptCount++;

						try
						{
							_Socket ??= CreateSocket(_RemoteEndPoint.AddressFamily);
							_Socket.Connect(_RemoteEndPoint);
						}
						catch (System.Net.Sockets.SocketException ex)
						{
							LastConnectionError = $"{ex.SocketErrorCode}: {ex.Message}";
							Nox.LogTrace.ErrorLine<Core.LogId.Net>(
								"Connect failed. Host={0} Port={1} SocketError={2} Message={3}",
								_InitContext.Hostname,
								_InitContext.Port,
								ex.SocketErrorCode,
								ex.Message);

							ResetConnectionAttempt();
							break;
						}
						catch (ObjectDisposedException ex)
						{
							LastConnectionError = $"{ex.GetType().Name}: {ex.Message}";
							Nox.LogTrace.WarningLine<Core.LogId.Net>("Connect socket was disposed. Recreating socket. {0}", ex);
							ResetConnectionAttempt();
							break;
						}
						catch (InvalidOperationException ex)
						{
							LastConnectionError = $"{ex.GetType().Name}: {ex.Message}";
							Nox.LogTrace.WarningLine<Core.LogId.Net>("Connect failed because socket state is invalid. Recreating socket. {0}", ex);
							ResetConnectionAttempt();
							break;
						}

						// 接続できたら NonBlocking にして、runtime 側の短い handshake 待ち時間内に最初の要求を送る。
						_Socket.Blocking = false;
						if (SendHandshake(HandShakeStr1) == false)
						{
							LastConnectionError = "Handshake1 send failed.";
							Nox.LogTrace.ErrorLine<Core.LogId.Net>("Handshake request send failed.");
							ResetConnectionAttempt();
							break;
						}

						_ConnectionState = ConnectionState.HandShake2;
						break;

					case ConnectionState.HandShake1:
						{
							System.Net.Sockets.Socket socket = _Socket ?? throw new InvalidOperationException("Socket is null in HandShake1 state.");
							// ハンドシェイク要求送信
							if (IsSendWritable(socket, TimeSpan.FromMilliseconds(10)) == false)
							{
								Nox.LogTrace.WarningLine<Core.LogId.Net>("Socket is not writable in HandShake1");
								break;
							}

							Span<byte> handShakeBuffer = stackalloc byte[HandShakeStr1.Length];
							Encoding.ASCII.GetBytes(HandShakeStr1, handShakeBuffer);
							if (this.Send(handShakeBuffer) == false)
							{
								Nox.LogTrace.ErrorLine<Core.LogId.Net>("Handshake request send failed.");
								ResetConnectionAttempt();
								break;
							}

							_ConnectionState = ConnectionState.HandShake2;
						}
						break;

					case ConnectionState.HandShake2:
						{
							// ハンドシェイク応答受信
							System.Net.Sockets.Socket socket = _Socket ?? throw new InvalidOperationException("Socket is null in HandShake2 state.");
							if (IsRecvReadable(socket, TimeSpan.FromMilliseconds(10)) == false)
							{
								//Nox.LogTrace.InfoLine<Core.LogId.Net>("Socket is not readable in HandShake2");
								break;
							}

							Span<byte> recvBuffer = stackalloc byte[HandShakeStr2.Length];
							if (this.ReceiveAll(recvBuffer) == false)
							{
								LastConnectionError = "Handshake2 receive failed.";
								Nox.LogTrace.ErrorLine<Core.LogId.Net>("Handshake response receive failed.");
								ResetConnectionAttempt();
								break;
							}

							string responseStr = Encoding.ASCII.GetString(recvBuffer);
							if (responseStr != HandShakeStr2)
							{
								LastConnectionError = $"Invalid Handshake2 response: {responseStr}";
								Nox.LogTrace.ErrorLine<Core.LogId.Net>("Invalid handshake response: {0}", responseStr);
								ResetConnectionAttempt();
								break;
							}

							if (SendHandshake(HandShakeStr3) == false)
							{
								LastConnectionError = "Handshake3 send failed.";
								Nox.LogTrace.ErrorLine<Core.LogId.Net>("Handshake request send failed.");
								ResetConnectionAttempt();
								break;
							}

							_ConnectionState = ConnectionState.Connected;
							LastConnectionError = string.Empty;
							Nox.LogTrace.InfoLine<Core.LogId.Net>("Connected to {0}:{1}", _InitContext.Hostname, _InitContext.Port);

							OnConneced();
						}
						break;

					case ConnectionState.HandShake3:
						{
							System.Net.Sockets.Socket socket = _Socket ?? throw new InvalidOperationException("Socket is null in HandShake3 state.");
							//	ハンドシェイクの送信　確立
							if (IsSendWritable(socket, TimeSpan.FromMilliseconds(10)) == false)
							{
								Nox.LogTrace.WarningLine<Core.LogId.Net>("Socket is not writable in HandShake1");
								break;
							}

							Span<byte> handShakeBuffer = stackalloc byte[HandShakeStr3.Length];
							Encoding.ASCII.GetBytes(HandShakeStr3, handShakeBuffer);
							if (this.Send(handShakeBuffer) == false)
							{
								Nox.LogTrace.ErrorLine<Core.LogId.Net>("Handshake request send failed.");
								ResetConnectionAttempt();
								break;
							}

							_ConnectionState = ConnectionState.Connected;
							Nox.LogTrace.InfoLine<Core.LogId.Net>("Connected to {0}:{1}", _InitContext.Hostname, _InitContext.Port);

							OnConneced();
						}
						break;

				}
			}
		}

		public void Update()
		{
			lock (_SocketSync)
			{
				switch (_ConnectionState)
				{
					case ConnectionState.Connected:
						Nox.Util.Assert(_Socket != null, "Socket is null in Connected state");

						//	切断検知: Poll(SelectRead) が true かつ Available==0 → 相手側が切断
						try
						{
							if (_Socket.Poll(0, System.Net.Sockets.SelectMode.SelectRead) && _Socket.Available == 0)
							{
								Nox.LogTrace.InfoLine<Core.LogId.Net>("Remote peer disconnected. {0}:{1}", _InitContext.Hostname, _InitContext.Port);
								HandleDisconnect();
								break;
							}
						}
						catch (System.Net.Sockets.SocketException ex)
						{
							Nox.LogTrace.ErrorLine<Core.LogId.Net>("Socket error during disconnect check: {0}", ex.SocketErrorCode);
							HandleDisconnect();
							break;
						}
						catch (ObjectDisposedException)
						{
							HandleDisconnect();
							break;
						}

						////	受信処理
						//if (IsRecvReadable(_Socket, TimeSpan.FromMicroseconds(10)))
						//{
						//	OnReceive();
						//}
						break;
				}
			}
		}

		protected virtual void OnReceive()
		{
		}

		public void Shutdown()
		{
			lock (_SocketSync)
			{
				if (_Socket == null)
				{
					_ConnectionState = ConnectionState.Disconnect;
				}
				else
				{
					Nox.LogTrace.InfoLine<Core.LogId.Net>("shutdown開始");

					try
					{
						if (_Socket.Connected)
						{
							_Socket.Shutdown(System.Net.Sockets.SocketShutdown.Both);
						}
					}
					catch
					{
						// ignore
					}

					try
					{
						_Socket.Close();
					}
					catch
					{
						// ignore
					}

					try
					{
						_Socket.Dispose();
					}
					catch
					{
						// ignore
					}

					_Socket = null;
					_PeerContext = default;
					_ConnectionState = ConnectionState.Disconnect;
				}
			}

			Core.StudioManager.Instance.GetEngineSystem<SocketScheduler>().UnregisterClient(this);
		}
		#endregion

		#region 非公開メソッド
		/// <summary>
		/// 接続確立
		/// </summary>
		protected virtual void OnConneced()
		{

		}

		protected virtual void OnDisconnected()
		{
		}

		private static System.Net.Sockets.Socket CreateSocket(System.Net.Sockets.AddressFamily addressFamily)
		{
			return new System.Net.Sockets.Socket(addressFamily, System.Net.Sockets.SocketType.Stream, System.Net.Sockets.ProtocolType.Tcp)
			{
				NoDelay = true,
				ReceiveTimeout = 5000,
				SendTimeout = 5000,
			};
		}

		private void ResetConnectionAttempt()
		{
			try
			{
				_Socket?.Close();
			}
			catch (ObjectDisposedException)
			{
			}

			_Socket?.Dispose();

			_Socket = _RemoteEndPoint == null ? null : CreateSocket(_RemoteEndPoint.AddressFamily);
			_ConnectionState = ConnectionState.Connection;
			_nextConnectAtUtc = DateTime.UtcNow.AddMilliseconds(ConnectRetryDelayMs);
		}

		private bool SendHandshake(string handshake)
		{
			System.Net.Sockets.Socket socket = _Socket ?? throw new InvalidOperationException("Socket is null while sending handshake.");
			if (IsSendWritable(socket, TimeSpan.FromMilliseconds(10)) == false)
			{
				return false;
			}

			Span<byte> handShakeBuffer = stackalloc byte[handshake.Length];
			Encoding.ASCII.GetBytes(handshake, handShakeBuffer);
			return Send(handShakeBuffer);
		}

		// 接続待ち（非ブロッキング＋Poll）
		private static bool ConnectWithTimeout(System.Net.Sockets.Socket socket, System.Net.EndPoint endPoint, int timeoutMs, out System.Net.Sockets.SocketError socketError)
		{
			socketError = System.Net.Sockets.SocketError.Success;
			bool originalBlocking = socket.Blocking;
			try
			{
				socket.Blocking = false;
				try
				{
					socket.Connect(endPoint);
					if (socket.Connected) return true;
				}
				catch (System.Net.Sockets.SocketException ex)
				{
					// 非ブロッキング接続の進行中は WouldBlock / InProgress
					if (ex.SocketErrorCode != System.Net.Sockets.SocketError.WouldBlock &&
						ex.SocketErrorCode != System.Net.Sockets.SocketError.InProgress &&
						ex.SocketErrorCode != System.Net.Sockets.SocketError.AlreadyInProgress)
					{
						socketError = ex.SocketErrorCode;
						return false;
					}
				}

				// 書き込み可能＝接続完了
				if (!socket.Poll(Math.Max(1, timeoutMs) * 1000, System.Net.Sockets.SelectMode.SelectWrite))
				{
					socketError = System.Net.Sockets.SocketError.TimedOut;
					return false;
				}

				// SO_ERROR 確認
				var soErrValue = socket.GetSocketOption(System.Net.Sockets.SocketOptionLevel.Socket, System.Net.Sockets.SocketOptionName.Error);
				int soErr = (soErrValue is int val) ? val : 0;
				socketError = (System.Net.Sockets.SocketError)soErr;
				return soErr == 0;
			}
			finally
			{
				try { socket.Blocking = originalBlocking; } catch { }
			}
		}

		/// <summary>
		/// 切断処理の共通ハンドラ
		/// </summary>
		private void HandleDisconnect()
		{
			if (_ConnectionState == ConnectionState.Disconnect)
			{
				return;
			}

			ConnectionState prevState = _ConnectionState;

			if (prevState == ConnectionState.Connected)
			{
				OnDisconnected();
			}

			ResetConnectionAttempt();
		}
		#endregion
	}
