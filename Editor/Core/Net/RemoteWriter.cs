using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Net
{
	public class RemoteWriter
	{
		#region 公開メソッド
		public RemoteWriter(int capacity = 8192)
		{
			Nox.Util.Assert(Nox.Util.IsPowOf(capacity, 2), "capacity must be power of 2.");
			_Buffer = new byte[capacity];
		}

		public void Write(ReadOnlySpan<byte> value)
		{

		}

		public unsafe void Write<T>(T value) where T : struct, System.Numerics.INumber<T>
		{
			const int align = 8; // アライメントを意識する場合は調整
			var size = sizeof(T);

			// バッファ終端をまたぐ場合は2回に分けて書き込む
			int first = Math.Min(size, _Buffer.Length - _Tail);
			Span<byte> dest = _Buffer.AsSpan(_Tail, first);

			// valueをバイト列に変換して書き込む
			unsafe
			{
				// スタック上に一時バッファを作成
				Span<byte> tmp = stackalloc byte[size];
				System.Runtime.InteropServices.MemoryMarshal.Write(tmp, ref value);
				tmp.Slice(0, first).CopyTo(dest);

				int remain = size - first;
				if (remain > 0)
				{
					// 先頭に折り返して残りを書き込む
					tmp.Slice(first, remain).CopyTo(_Buffer.AsSpan(0, remain));
				}
			}

			_Tail = (_Tail + size) & (_Buffer.Length - 1);
			// _Head, _Tail だけで管理する場合は「バッファ満杯時は1バイト空ける」運用が必要
			// ここでは「満杯＝書き込み不可」としている
		}

		//		public ReadOnlySpan<byte> Get
		#endregion

		#region 非公開フィールド
		private readonly byte[] _Buffer;
		private int _Head = 0;
		private int _Tail = 0;
		#endregion
	}
}
