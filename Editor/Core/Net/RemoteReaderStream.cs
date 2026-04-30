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
     private readonly object _Lock = new();
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
           get { lock (_Lock) { return _Head; } }
			set => throw new NotSupportedException();
		}
		#endregion

		#region 公開プロパティ
		/// <summary>読み取り可能なバイト数</summary>
       public int Available { get { lock (_Lock) { return GetAvailableNoLock(); } } }

		/// <summary>書き込み可能な残りバイト数</summary>
     public int FreeBytes { get { lock (_Lock) { return GetFreeBytesNoLock(); } } }

		/// <summary>バッファが空か</summary>
      public bool IsEmpty { get { lock (_Lock) { return _Head == _Tail; } } }
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

		public void AddBuffer(ReadOnlySpan<byte> buffer)
		{
           lock (_Lock)
			{
				//	bufferをリングバッファに追加
				Nox.Util.Assert(buffer.Length <= GetFreeBytesNoLock(), "Buffer overflow: not enough free space in ring buffer.");

             // Tail からバッファ末尾までの連続領域
				int firstPart = Math.Min(buffer.Length, _Buffer.Length - _Tail);
				buffer.Slice(0, firstPart).CopyTo(_Buffer.AsSpan(_Tail, firstPart));

               // 折り返し部分
				int secondPart = buffer.Length - firstPart;
				if (secondPart > 0)
				{
					buffer.Slice(firstPart, secondPart).CopyTo(_Buffer.AsSpan(0, secondPart));
				}

                _Tail = (_Tail + buffer.Length) & _Mask;
			}

		}

		/// <summary>
		/// 指定オフセットのバイトを消費せずに覗き見る
		/// </summary>
		private byte PeekByte(int offset)
		{
			return _Buffer[(_Head + offset) & _Mask];
		}

		/// <summary>
		/// 1packet 読み取れるか 
		/// </summary>
		/// <returns></returns>
		public bool CanReadBody()
		{
            lock (_Lock)
			{
                long packetSize = 0;
				int headerBytes = 0;
				int shift = 0;

                // LEB128 for uint64 は最大 10 バイト
				for (int i = 0; i < 10; ++i)
				{
					if (i >= GetAvailableNoLock())
                   {
						return false; // LEB128 ヘッダが揃っていない
					}

                    byte b = PeekByte(i);
					packetSize |= (long)(b & 0x7F) << shift;
					shift += 7;
					++headerBytes;

                    if ((b & 0x80) == 0)
					{
						// ヘッダ + パケット全体が揃っているか
						return packetSize <= _Buffer.Length && GetAvailableNoLock() >= headerBytes + (int)packetSize;
					}
				}

                return false; // 不正な LEB128
			}
		}

		/// <summary>
		/// BinaryReader から呼ばれる（Span 版）
		/// </summary>
		public override int Read(Span<byte> buffer)
		{
          lock (_Lock)
			{
                int readSize = Math.Min(buffer.Length, GetAvailableNoLock());
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
         lock (_Lock)
			{
                if (_Head == _Tail) return -1;

                byte value = _Buffer[_Head];
				_Head = (_Head + 1) & _Mask;
				return value;
			}
		}

		/// <summary>
		/// バッファをクリア
		/// </summary>
		public void Clear()
		{
          lock (_Lock)
			{
                _Head = 0;
				_Tail = 0;
			}
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

		private int GetAvailableNoLock() => (_Tail - _Head + _Buffer.Length) & _Mask;
		private int GetFreeBytesNoLock() => _Buffer.Length - GetAvailableNoLock() - 1;
	}
}
