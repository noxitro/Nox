using System;
using System.Buffers.Binary;

namespace Nox
{
	public ref struct StackMemoryWriter
	{
		#region 非公開フィールド
		private Span<byte> _Buffer;
		private int _Position;
		#endregion

		#region 公開プロパティ
		public readonly int Position => _Position;
		public readonly int Length => _Buffer.Length;
		public readonly bool IsEnd => _Position >= _Buffer.Length;

		/// <summary>書き込み済み領域</summary>
		public readonly ReadOnlySpan<byte> WrittenSpan => _Buffer.Slice(0, _Position);
		#endregion

		#region 公開メソッド
		public StackMemoryWriter(Span<byte> buffer)
		{
			_Buffer = buffer;
			_Position = 0;
		}

		public void WriteBytes(scoped ReadOnlySpan<byte> data)
		{
			if (data.Length > _Buffer.Length - _Position)
			{
				throw new InvalidOperationException(
					$"Buffer overflow. Position={_Position}, WriteSize={data.Length}, Capacity={_Buffer.Length}");
			}
			data.CopyTo(_Buffer.Slice(_Position));
			_Position += data.Length;
		}

		public void Write(bool value) => Write((byte)(value ? 1 : 0));

		public void Write(byte value)
		{
			if (_Position >= _Buffer.Length)
				throw new InvalidOperationException(
					$"Buffer overflow. Position={_Position}, Capacity={_Buffer.Length}");
			_Buffer[_Position++] = value;
		}

		public void Write(sbyte value) => Write((byte)value);

		public void Write(short value)
		{
			Span<byte> temp = stackalloc byte[sizeof(short)];
			BinaryPrimitives.WriteInt16LittleEndian(temp, value);
			WriteBytes(temp);
		}

		public void Write(ushort value)
		{
			Span<byte> temp = stackalloc byte[sizeof(ushort)];
			BinaryPrimitives.WriteUInt16LittleEndian(temp, value);
			WriteBytes(temp);
		}

		public void Write(int value)
		{
			Span<byte> temp = stackalloc byte[sizeof(int)];
			BinaryPrimitives.WriteInt32LittleEndian(temp, value);
			WriteBytes(temp);
		}

		public void Write(uint value)
		{
			Span<byte> temp = stackalloc byte[sizeof(uint)];
			BinaryPrimitives.WriteUInt32LittleEndian(temp, value);
			WriteBytes(temp);
		}

		public void Write(long value)
		{
			Span<byte> temp = stackalloc byte[sizeof(long)];
			BinaryPrimitives.WriteInt64LittleEndian(temp, value);
			WriteBytes(temp);
		}

		public void Write(ulong value)
		{
			Span<byte> temp = stackalloc byte[sizeof(ulong)];
			BinaryPrimitives.WriteUInt64LittleEndian(temp, value);
			WriteBytes(temp);
		}

		public void Write(float value)
		{
			Span<byte> temp = stackalloc byte[sizeof(float)];
			BinaryPrimitives.WriteSingleLittleEndian(temp, value);
			WriteBytes(temp);
		}

		public void Write(double value)
		{
			Span<byte> temp = stackalloc byte[sizeof(double)];
			BinaryPrimitives.WriteDoubleLittleEndian(temp, value);
			WriteBytes(temp);
		}

		public void Write(ReadOnlySpan<char> value)
		{
			// UTF-8 エンコード（スタック上に一時バッファ）
			int maxByteCount = System.Text.Encoding.UTF8.GetMaxByteCount(value.Length);
			Span<byte> encoded = stackalloc byte[maxByteCount];
			int actualByteCount = System.Text.Encoding.UTF8.GetBytes(value, encoded);

			// LEB128 でバイト数を先行書き込み（BinaryWriter.Write(string) 互換）
			WriteLeb128((uint)actualByteCount);

			// UTF-8 バイト列を書き込み
			WriteBytes(encoded.Slice(0, actualByteCount));
		}

		/// <summary>
		/// LEB128（7ビット符号なし整数エンコード）を書き込む
		/// BinaryWriter.Write7BitEncodedInt と同形式
		/// </summary>
		public void WriteLeb128(uint value)
		{
			do
			{
				byte b = (byte)(value & 0x7F);
				value >>= 7;
				if (value != 0) b |= 0x80;
				Write(b);
			} while (value != 0);
		}
		#endregion
	}
}
