using System;
using System.Buffers.Binary;

namespace Nox
{
	public ref struct StackMemoryStreamReader
	{
		#region 非公開フィールド
		private readonly ReadOnlySpan<byte> _Buffer;
		private int _Position;
		#endregion

		#region 公開プロパティ
		public readonly int Position => _Position;
		public readonly int Length => _Buffer.Length;
		public readonly bool IsEnd => _Position >= _Buffer.Length;
		public readonly int Remaining => _Buffer.Length - _Position;
		#endregion

		#region 公開メソッド
		public StackMemoryStreamReader(ReadOnlySpan<byte> buffer)
		{
			_Buffer = buffer;
			_Position = 0;
		}

		public ReadOnlySpan<byte> ReadBytes(int size)
		{
			if (size > _Buffer.Length - _Position)
			{
				throw new InvalidOperationException(
					$"Buffer underflow. Position={_Position}, ReadSize={size}, Capacity={_Buffer.Length}");
			}
			ReadOnlySpan<byte> result = _Buffer.Slice(_Position, size);
			_Position += size;
			return result;
		}

		public bool   ReadBool()  => ReadByte() != 0;

		public byte ReadByte()
		{
			if (_Position >= _Buffer.Length)
				throw new InvalidOperationException(
					$"Buffer underflow. Position={_Position}, Capacity={_Buffer.Length}");
			return _Buffer[_Position++];
		}

		public sbyte  ReadSByte()  => (sbyte)ReadByte();

		public short ReadInt16()
		{
			short result = BinaryPrimitives.ReadInt16LittleEndian(_Buffer.Slice(_Position));
			_Position += sizeof(short);
			return result;
		}

		public ushort ReadUInt16()
		{
			ushort result = BinaryPrimitives.ReadUInt16LittleEndian(_Buffer.Slice(_Position));
			_Position += sizeof(ushort);
			return result;
		}

		public int ReadInt32()
		{
			int result = BinaryPrimitives.ReadInt32LittleEndian(_Buffer.Slice(_Position));
			_Position += sizeof(int);
			return result;
		}

		public uint ReadUInt32()
		{
			uint result = BinaryPrimitives.ReadUInt32LittleEndian(_Buffer.Slice(_Position));
			_Position += sizeof(uint);
			return result;
		}

		public long ReadInt64()
		{
			long result = BinaryPrimitives.ReadInt64LittleEndian(_Buffer.Slice(_Position));
			_Position += sizeof(long);
			return result;
		}

		public ulong ReadUInt64()
		{
			ulong result = BinaryPrimitives.ReadUInt64LittleEndian(_Buffer.Slice(_Position));
			_Position += sizeof(ulong);
			return result;
		}

		public float ReadFloat()
		{
			float result = BinaryPrimitives.ReadSingleLittleEndian(_Buffer.Slice(_Position));
			_Position += sizeof(float);
			return result;
		}

		public double ReadDouble()
		{
			double result = BinaryPrimitives.ReadDoubleLittleEndian(_Buffer.Slice(_Position));
			_Position += sizeof(double);
			return result;
		}

		/// <summary>
		/// LEB128（7ビット符号なし整数）を読み込む
		/// StackMemoryStreamWriter.WriteLeb128 と対応
		/// </summary>
		public uint ReadLeb128()
		{
			uint value = 0;
			int shift = 0;
			while (true)
			{
				byte b = ReadByte();
				value |= (uint)(b & 0x7F) << shift;
				if ((b & 0x80) == 0) break;
				shift += 7;
			}
			return value;
		}

		/// <summary>
		/// LEB128 長さプレフィックス + UTF-8 バイト列を読み込む
		/// StackMemoryStreamWriter.Write(ReadOnlySpan&lt;char&gt;) と対応
		/// </summary>
		/// <param name="output">デコード先の文字バッファ（stackalloc char[N] を推奨）</param>
		/// <returns>書き込んだ文字数</returns>
		public int ReadString(Span<char> output)
		{
			uint byteCount = ReadLeb128();
			ReadOnlySpan<byte> utf8Bytes = ReadBytes((int)byteCount);
			return System.Text.Encoding.UTF8.GetChars(utf8Bytes, output);
		}

		public string ReadString()
		{
			BinaryReader b;
			uint byteCount = ReadLeb128();
			ReadOnlySpan<byte> utf8Bytes = ReadBytes((int)byteCount);
			return System.Text.Encoding.UTF8.GetString(utf8Bytes);
		}
		#endregion
	}
}
