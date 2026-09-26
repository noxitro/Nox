// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Net;

	public enum SendFlag : byte
	{
		None,
	}

	public abstract class Entity
	{
		#region 公開メソッド
		public bool Send(ReadOnlySpan<byte> buffer, SendFlag flags = SendFlag.None)
		{
			Nox.Util.Assert(_Socket != null, "Socket is null.");

			int remainSize = buffer.Length;
			while (remainSize > 0)
			{
				try
				{
					int sentSize = _Socket.Send(buffer.Slice(buffer.Length - remainSize, remainSize));
					Nox.LogTrace.InfoLine<Core.LogId.Net>("Sent {0} bytes.", sentSize);
					if (sentSize > 0)
					{
						remainSize -= sentSize;
					}
					else if (sentSize == 0)
					{
						//	切断された
						return false;
					}
					else
					{
						Nox.Util.Assert(false, "Unexpected negative sent size.");
					}
				}
				catch (System.Net.Sockets.SocketException ex) when (ex.SocketErrorCode == System.Net.Sockets.SocketError.WouldBlock)
				{
					// バッファに空きがないため待機して再試行
					if (!IsSendWritable(_Socket, TimeSpan.FromMilliseconds(100)))
					{
						Nox.LogTrace.ErrorLine<Core.LogId.Net>("Send timed out waiting for writable socket.");
						return false;
					}
				}
				catch (System.Net.Sockets.SocketException ex)
				{
					Nox.LogTrace.ErrorLine<Core.LogId.Net>("Send failed. SocketError={0} Message={1}", ex.SocketErrorCode, ex.Message);
					return false;
				}
			}

			return true;
		}

		public bool ReceiveAll(Span<byte> buffer)
		{
			Nox.Util.Assert(_Socket != null, "Socket is null.");

			int remainSize = buffer.Length;
			while (remainSize > 0)
			{
				try
				{
					int receivedSize = _Socket.Receive(buffer.Slice(buffer.Length - remainSize, remainSize));
					Nox.LogTrace.InfoLine<Core.LogId.Net>("Received {0} bytes.", receivedSize);
					if (receivedSize > 0)
					{
						remainSize -= receivedSize;
					}
					else if (receivedSize == 0)
					{
						//	切断された
						return false;
					}
				}
				catch (System.Net.Sockets.SocketException ex) when (ex.SocketErrorCode == System.Net.Sockets.SocketError.WouldBlock)
				{
					// バッファに空きがないため待機して再試行
					if (!IsRecvReadable(_Socket, TimeSpan.FromMilliseconds(100)))
					{
						Nox.LogTrace.ErrorLine<Core.LogId.Net>("Send timed out waiting for writable socket.");
						return false;
					}
				}
				catch (System.Net.Sockets.SocketException ex)
				{
					Nox.LogTrace.ErrorLine<Core.LogId.Net>("receive failed. SocketError={0} Message={1}", ex.SocketErrorCode, ex.Message);
					return false;
				}
			}
			return true;
		}

		public int Receive(Span<byte> buffer)
		{
			Nox.Util.Assert(_Socket != null, "Socket is null.");
			try
			{
				// Available: OSのソケット受信バッファに溜まっているバイト数
				int available = _Socket.Available;
				if (available == 0)
				{
					return 0;
				}

          // バッファサイズと Available の小さい方で受信
				int receiveSize = Math.Min(available, buffer.Length);
				int receivedSize = _Socket.Receive(buffer.Slice(0, receiveSize));

				if (receivedSize == 0)
				{
					return -1; // 切断
				}
				return receivedSize;
			}
			catch (System.Net.Sockets.SocketException ex)
				when (ex.SocketErrorCode == System.Net.Sockets.SocketError.WouldBlock)
			{
				return 0;
			}
			catch (System.Net.Sockets.SocketException ex)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Net>(
					"Receive failed. SocketError={0} Message={1}", ex.SocketErrorCode, ex.Message);
				return -1;
			}
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

		public bool IsRecvReadable(int millisecondsTimeout)
		{
			Nox.Util.Assert(_Socket != null, "Socket is null.");
			try
			{
				return _Socket.Poll(millisecondsTimeout, System.Net.Sockets.SelectMode.SelectRead);
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

		public static bool CheckDisconnected(System.Net.Sockets.Socket socket)
		{
			try
			{
				return socket.Poll(0, System.Net.Sockets.SelectMode.SelectRead) && socket.Available == 0;
			}
			catch
			{
				return true;
			}
		}
		#endregion

		#region 非公開フィールド


		protected System.Net.Sockets.Socket? _Socket;
		#endregion
	}
