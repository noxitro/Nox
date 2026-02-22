using System;

namespace Core.Net
{
	/// <summary>
	/// ソケット受信用の読み取り専用リングバッファストリーム
	/// BinaryReader から利用される
	/// </summary>
	public sealed class RemoteReaderStream : System.IO.Stream
	{
		#region 非公開フィールド
		private readonly RuntimeRemoteClient _Client;
		private readonly byte[] _Buffer;
		private readonly int _Mask;
		private int _Head;  // 読み取り位置
		private int _Tail;  // 書き込み位置（受信データの末尾）
		#endregion

		#region Stream プロパティ
		public override bool CanRead => true;
		public override bool CanSeek => false;
		public override bool CanWrite => false;
		public override long Length => Available;
		public override long Position
		{
			get => _Head;
			set => throw new NotSupportedException();
		}
		#endregion

		#region 公開プロパティ
		/// <summary>読み取り可能なバイト数</summary>
		public int Available => (_Tail - _Head + _Buffer.Length) & _Mask;

		/// <summary>書き込み可能な残りバイト数</summary>
		public int FreeBytes => _Buffer.Length - Available - 1;

		/// <summary>バッファが空か</summary>
		public bool IsEmpty => _Head == _Tail;
		#endregion

		#region コンストラクタ
		public RemoteReaderStream(RuntimeRemoteClient client, int capacity = 8192)
		{
			Nox.Util.Assert(Nox.Util.IsPowOf(capacity, 2), "capacity must be power of 2.");
			_Client = client;
			_Buffer = new byte[capacity];
			_Mask = capacity - 1;
		}
		#endregion

		#region 公開メソッド

		/// <summary>
		/// ソケットからデータを受信してリングバッファに追加
		/// </summary>
		public int Fill()
		{
			if (FreeBytes == 0) return 0;

			// リングバッファの空き領域に直接受信
			int totalReceived = 0;

			// Tail から末尾までの連続領域
			int firstPart = Math.Min(FreeBytes, _Buffer.Length - _Tail);
			if (firstPart > 0)
			{
				var error = _Client.Receive(_Buffer.AsSpan(_Tail, firstPart));
				if (error.HasValue)
				{
					Nox.LogTrace.ErrorLine<Core.LogId.Net>("Receive failed in Fill (first part)");
					return totalReceived;
				}
				totalReceived += firstPart;
				_Tail = (_Tail + firstPart) & _Mask;
			}

			return totalReceived;
		}

		/// <summary>
		/// BinaryReader から呼ばれる（Span 版）
		/// </summary>
		public override int Read(Span<byte> buffer)
		{
			int readSize = Math.Min(buffer.Length, Available);
			if (readSize == 0) return 0;

			// Head から末尾までの連続領域
			int firstPart = Math.Min(readSize, _Buffer.Length - _Head);
			_Buffer.AsSpan(_Head, firstPart).CopyTo(buffer);

			// 折り返し部分
			int secondPart = readSize - firstPart;
			if (secondPart > 0)
			{
				_Buffer.AsSpan(0, secondPart).CopyTo(buffer.Slice(firstPart));
			}

			_Head = (_Head + readSize) & _Mask;
			return readSize;
		}

		/// <summary>
		/// BinaryReader.ReadString 等から呼ばれる可能性がある（byte[] 版）
		/// </summary>
		public override int Read(byte[] buffer, int offset, int count)
		{
			return Read(buffer.AsSpan(offset, count));
		}

		/// <summary>
		/// 1 バイト読み取り
		/// </summary>
		public override int ReadByte()
		{
			if (IsEmpty) return -1;

			byte value = _Buffer[_Head];
			_Head = (_Head + 1) & _Mask;
			return value;
		}

		/// <summary>
		/// バッファをクリア
		/// </summary>
		public void Clear()
		{
			_Head = 0;
			_Tail = 0;
		}

		#endregion

		#region 未サポート操作
		public override void Flush()
		{
			// 読み取り専用なので何もしない
		}

		public override void Write(byte[] buffer, int offset, int count)
			=> throw new NotSupportedException("Write not supported");

		public override long Seek(long offset, SeekOrigin origin)
			=> throw new NotSupportedException("Seek not supported");

		public override void SetLength(long value)
			=> throw new NotSupportedException("SetLength not supported");
		#endregion
	}
}
