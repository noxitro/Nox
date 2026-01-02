using System;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace Core.Net
{
	public class Client : Core.Net.Entity
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
			HandShake1,
			HandSHake2,
			HandShake3,
			Connected,
		}
		#endregion

		#region 非公開フィールド
		private const string HandshakeResponsePrefix = "NOX_RUNTIME_HANDSHAKE ";
		private const int ReceiveBufferSize = 4096;
		private const int HandshakeMaxLineLength = 512;

		private static readonly byte[] s_handshakeRequest = Encoding.UTF8.GetBytes("NOX_EDITOR_HANDSHAKE\n");
		private static readonly byte[] s_handshakeAck = Encoding.UTF8.GetBytes("NOX_EDITOR_HANDSHAKE_ACK\n");

		private readonly byte[] _receiveBuffer = new byte[ReceiveBufferSize];
		private Socket? _Socket;
		private PeerContext _PeerContext;
		private ConnectionState _ConnectionState = ConnectionState.Disconnect;
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
			
			_Socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp)
			{
				NoDelay = true,
				ReceiveTimeout = 5000,
				SendTimeout = 5000,
			};

			_Socket.Blocking = false;

			try
			{
				_Socket.Connect(context.Hostname, context.Port);
			}
			catch
			{
				Shutdown();
				throw;
			}
		}

		public void PollAccept()
		{
			if (!IsConnected || _Socket == null)
			{
				return;
			}

			try
			{
				while (_Socket.Available > 0)
				{
					int read = _Socket.Receive(_receiveBuffer, 0, _receiveBuffer.Length, SocketFlags.None);
					if (read <= 0)
					{
						Shutdown();
						return;
					}

					//Received?.Invoke(new ReadOnlyMemory<byte>(_receiveBuffer, 0, read));
				}
			}
			catch (SocketException)
			{
				Shutdown();
			}
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
					_Socket.Shutdown(SocketShutdown.Both);
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
		#endregion

		#region 非公開メソッド
		#endregion
	}
}
