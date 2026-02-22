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
            if (_Position > 0)
            {
                _Client.Send(_Buffer.AsSpan(0, _Position));
                _Position = 0;
            }
        }

        /// <summary>
        /// バッファをクリア（送信せずに破棄）
        /// </summary>
        public void Clear()
        {
           _Position = 0;
        }

		public override void Write(byte[] buffer, int offset, int count)
		{
			// string 書き込み時に呼ばれる可能性があるため委譲
			Write(buffer.AsSpan(offset, count));
		}

		#endregion

		#region 非公開メソッド
		private void WriteLength(uint value)
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
