using System;
using System.Globalization;
using System.IO;
using System.Text;

namespace Core.Net
{
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

		#endregion

		#region 公開プロパティ
		public bool IsConnected => _ConnectionState == ConnectionState.Connected && _Socket?.Connected == true;
		public ConnectionState State => _ConnectionState;
		public PeerContext Peer => _PeerContext;
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
			_InitContext = context;

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
			_RemoteEndPoint = new System.Net.IPEndPoint(address, port);

			_Socket = new System.Net.Sockets.Socket(address.AddressFamily, System.Net.Sockets.SocketType.Stream, System.Net.Sockets.ProtocolType.Tcp)
			{
				NoDelay = true,
				ReceiveTimeout = 5000,
				SendTimeout = 5000,
			};

			_ConnectionState = ConnectionState.Connection;

			SocketScheduler.Instance.RegisterClient(this);
		}

		void IDisposable.Dispose()
		{
			Shutdown();
		}

		public void Connection()
		{
			if (_ConnectionState == ConnectionState.Connected)
			{
				return;
			}

			Nox.Util.Assert(_Socket != null, "Socket is null in Connection state");
			Nox.Util.Assert(_RemoteEndPoint != null, "_LocalEndPoint is null in Connection state");

			switch (_ConnectionState)
			{
				case ConnectionState.Connection:
					// 次の試行時刻までは何もしない
					if (DateTime.UtcNow < _nextConnectAtUtc)
					{
						return;
					}

					try
					{
						_Socket.Connect(_RemoteEndPoint);
					}
					catch (System.Net.Sockets.SocketException ex)
					{
						Nox.LogTrace.ErrorLine<Core.LogId.Net>(
							"Connect failed. Host={0} Port={1} SocketError={2} Message={3}",
							_InitContext.Hostname,
							_InitContext.Port,
							ex.SocketErrorCode,
							ex.Message);

						_nextConnectAtUtc = DateTime.UtcNow.AddMilliseconds(ConnectRetryDelayMs);
						break;
					}

					// 接続できたら NonBlocking にして Poll で処理
					_Socket.Blocking = false;
					_ConnectionState = ConnectionState.HandShake1;
					break;

				case ConnectionState.HandShake1:
					{
						// ハンドシェイク要求送信
						if (IsSendWritable(_Socket, TimeSpan.FromMilliseconds(10)) == false)
						{
							Nox.LogTrace.WarningLine<Core.LogId.Net>("Socket is not writable in HandShake1");
							break;
						}

						Span<byte> handShakeBuffer = stackalloc byte[HandShakeStr1.Length];
						Encoding.ASCII.GetBytes(HandShakeStr1, handShakeBuffer);
						if (this.Send(handShakeBuffer).HasValue)
						{
							Nox.LogTrace.ErrorLine<Core.LogId.Net>("Handshake request send failed.");
							break;
						}

						_ConnectionState = ConnectionState.HandShake2;
					}
					break;

				case ConnectionState.HandShake2:
					// ハンドシェイク応答受信

					if (IsRecvReadable(_Socket, TimeSpan.FromMilliseconds(10)) == false)
					{
						//Nox.LogTrace.InfoLine<Core.LogId.Net>("Socket is not readable in HandShake2");
						break;
					}

					Span<byte> recvBuffer = stackalloc byte[HandShakeStr2.Length];
					if (this.Receive(recvBuffer).HasValue)
					{
						Nox.LogTrace.ErrorLine<Core.LogId.Net>("Handshake response receive failed.");
						break;
					}

					string responseStr = Encoding.ASCII.GetString(recvBuffer);
					if (responseStr != HandShakeStr2)
					{
						Nox.LogTrace.ErrorLine<Core.LogId.Net>("Invalid handshake response: {0}", responseStr);
						break;
					}

					_ConnectionState = ConnectionState.HandShake3;
					break;

				case ConnectionState.HandShake3:
					{
						//	ハンドシェイクの送信　確立
						if (IsSendWritable(_Socket, TimeSpan.FromMilliseconds(10)) == false)
						{
							Nox.LogTrace.WarningLine<Core.LogId.Net>("Socket is not writable in HandShake1");
							break;
						}

						Span<byte> handShakeBuffer = stackalloc byte[HandShakeStr3.Length];
						Encoding.ASCII.GetBytes(HandShakeStr3, handShakeBuffer);
						if (this.Send(handShakeBuffer).HasValue)
						{
							Nox.LogTrace.ErrorLine<Core.LogId.Net>("Handshake request send failed.");
							break;
						}

						_ConnectionState = ConnectionState.Connected;
						Nox.LogTrace.InfoLine<Core.LogId.Net>("Connected to {0}:{1}", _InitContext.Hostname, _InitContext.Port);
					}
					break;

			}
		}

		public void Update()
		{
			switch (_ConnectionState)
			{
				case ConnectionState.Connected:
					Nox.Util.Assert(_Socket != null, "Socket is null in Connected state");
					//	受信処理
					if (IsRecvReadable(_Socket, TimeSpan.FromMicroseconds(10)))
					{
						OnReceive();
					}
					break;
			}
		}

		protected virtual void OnReceive()
		{
		}

		public void Shutdown()
		{
			if (_Socket == null)
			{
				return;
			}

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

			SocketScheduler.Instance.UnregisterClient(this);
		}
		#endregion

		#region 非公開メソッド
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
		#endregion
	}
}
