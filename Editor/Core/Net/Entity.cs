using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Net
{
	public struct SocketIoError
	{
		public enum Kind : byte
		{

		}
	}

	public enum SendFlag : byte
	{
		None,
	}

	public abstract class Entity
	{
		#region 公開メソッド
		public SocketIoError? Send(ReadOnlySpan<byte> buffer, SendFlag flags = SendFlag.None)
		{
			if (_Socket == null)
			{
				throw new InvalidOperationException("Socket is null.");
			}
			return Send(_Socket, buffer, flags);
		}

		public static SocketIoError? Send(System.Net.Sockets.Socket socket, ReadOnlySpan<byte> buffer, SendFlag flags = SendFlag.None)
		{
			int remainSize = buffer.Length;
			while (remainSize > 0)
			{
				try
				{
					int sentSize = socket.Send(buffer.Slice(buffer.Length - remainSize, remainSize));
					if (sentSize <= 0)
					{
						return new SocketIoError() { };
					}
					remainSize -= sentSize;
				}
				catch (System.Net.Sockets.SocketException ex)
				{
					return new SocketIoError() { };
				}
			}

			return null;
		}

		public SocketIoError? Receive(Span<byte> buffer)
		{
			if (_Socket == null)
			{
				throw new InvalidOperationException("Socket is null.");
			}
			return Receive(_Socket, buffer);
		}

		public static SocketIoError? Receive(System.Net.Sockets.Socket socket, Span<byte> buffer)
		{
			int remainSize = buffer.Length;
			while (remainSize > 0)
			{
				try
				{
					int receivedSize = socket.Receive(buffer.Slice(buffer.Length - remainSize, remainSize));
					if (receivedSize <= 0)
					{
						return new SocketIoError() { };
					}
					remainSize -= receivedSize;
				}
				catch (System.Net.Sockets.SocketException ex)
				{
					return new SocketIoError() { };
				}
			}
			return null;
		}

		public static bool IsSendWritable(System.Net.Sockets.Socket socket, in TimeSpan timeout)
		{
			try
			{
				return socket.Poll(timeout, System.Net.Sockets.SelectMode.SelectWrite);
			}
			catch
			{
				return false;
			}
		}

		public static bool IsRecvReadable(System.Net.Sockets.Socket socket, in TimeSpan timeout)
		{
			try
			{
				return socket.Poll(timeout, System.Net.Sockets.SelectMode.SelectRead);
			}
			catch
			{
				return false;
			}
		}
		#endregion

		#region 非公開フィールド


		protected System.Net.Sockets.Socket? _Socket;
		#endregion
	}
}
