using System;

namespace Core.Net
{
    /// <summary>
    /// ソケット送信用の書き込み専用リングバッファストリーム
    /// </summary>
    public sealed class RemoteWriterStream : System.IO.Stream
    {
        #region 非公開フィールド
        private readonly RuntimeRemoteClient _Client;
        private readonly byte[] _Buffer;
        private int _Position;

		// LEB128(uint64)の最大バイト数。先頭に予約しておく
		private const int HeaderReservedBytes = 10;
		#endregion

		#region 公開プロパティ
		public override bool CanRead => false;
		public override bool CanSeek => false;
		public override bool CanWrite => true;
		public override long Length => _Position;
		public override long Position
		{
			get => _Position;
			set => throw new NotSupportedException();
		}
		#endregion

		#region コンストラクタ
		public RemoteWriterStream(RuntimeRemoteClient client, int capacity = 8192)
        {
            Nox.Util.Assert(Nox.Util.IsPowOf(capacity, 2), "capacity must be power of 2.");
            _Client = client;
            _Buffer = new byte[capacity];
			_Position = HeaderReservedBytes; // 先頭に LEB128 の最大バイト数分を予約
		}
		#endregion

		#region 公開メソッド
		public override void Write(ReadOnlySpan<byte> buffer)
		{
            int writeSize = buffer.Length;
			if (_Position + writeSize > _Buffer.Length)
			{
				Nox.Util.Assert(false, "Buffer overflow. Position={0}, WriteSize={1}, Capacity={2}",
					_Position, writeSize, _Buffer.Length);
				throw new InvalidOperationException("Buffer overflow");
			}

			_Position += writeSize;

			buffer.CopyTo(new Span<byte>(_Buffer, _Position - writeSize, writeSize));
		}

		/// <summary>
		/// バッファの内容をソケットに送信
		/// </summary>
		public override void Flush()
		{
			int payloadSize = _Position - HeaderReservedBytes;
			if (payloadSize <= 0)
			{
				return;
			}

			// LEB128 を予約領域の末尾から逆方向に書く（右詰め）
			int headerBytes = WriteLeb128ToEnd(HeaderReservedBytes, (uint)payloadSize);
			int sendStart = HeaderReservedBytes - headerBytes;

         if (_Client.Send(_Buffer.AsSpan(sendStart, headerBytes + payloadSize)) == false)
			{
				throw new InvalidOperationException("Socket send failed");
			}
			_Position = HeaderReservedBytes;
		}

		/// <summary>
		/// バッファをクリア（送信せずに破棄）
		/// </summary>
		public void Clear()
        {
          _Position = HeaderReservedBytes;
        }

		public override void Write(byte[] buffer, int offset, int count)
		{
			// string 書き込み時に呼ばれる可能性があるため委譲
			Write(buffer.AsSpan(offset, count));
		}

		#endregion

		#region 非公開メソッド
		private void WriteLength(ulong value)
        {
			do
			{
				byte b = (byte)(value & 0x7F);  // 下位 7 ビット
				value >>= 7;

				if (value != 0)
				{
					b |= 0x80;  // 継続ビット
				}

				if (_Position >= _Buffer.Length)
				{
					throw new InvalidOperationException("Buffer overflow");
				}

				_Buffer[_Position++] = b;

			} while (value != 0);
		}

		/// <summary>
		/// LEB128 を予約領域に右詰めで書き込む
		/// </summary>
		/// <returns>書き込んだバイト数</returns>
		private int WriteLeb128ToEnd(int reservedEnd, ulong value)
		{
			// 正順（LSB ファースト）で一時バッファに書く
			Span<byte> temp = stackalloc byte[HeaderReservedBytes];
			int count = 0;
			do
			{
				byte b = (byte)(value & 0x7F);
				value >>= 7;
				if (value != 0) b |= 0x80;
				temp[count++] = b;
			} while (value != 0);

			// 予約領域の末尾に右詰めでコピー
			int startPos = reservedEnd - count;
			temp.Slice(0, count).CopyTo(_Buffer.AsSpan(startPos, count));
			return count;
		}
		#endregion

		#region 未サポート操作
		public override int Read(byte[] buffer, int offset, int count)
            => throw new NotSupportedException("Read not supported");

        public override long Seek(long offset, SeekOrigin origin)
            => throw new NotSupportedException("Seek not supported");

        public override void SetLength(long value)
            => throw new NotSupportedException("SetLength not supported");

		#endregion
	}
}
