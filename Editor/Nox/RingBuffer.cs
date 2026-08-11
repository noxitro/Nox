using System;
using System.Collections.Generic;
using System.Text;

namespace Nox;

	public struct RingBuffer<T> : IEnumerable<T>
	{
		#region 非公開フィールド
		private readonly T[] _Buffer;
		private int _Head = 0;
		private int _Tail = 0;
		#endregion

		#region プロパティ
		public readonly int Mask => _Buffer.Length - 1;
		public readonly int Count => _Buffer.Length;

		public readonly ref T this[int index]
		{
			get
			{
				return ref _Buffer[(index + _Head) & Mask];
			}
		}
		#endregion

		#region 公開メソッド
		public RingBuffer(int capacity)
		{
			_Buffer = new T[capacity];
		}

		public void Add(T value)
		{
			_Buffer[_Head++] = value;
		}

		public IEnumerator<T> GetEnumerator()
		{
			if (_Head <= _Tail)
			{
				for (int i = _Head; i < _Tail; ++i)
					yield return _Buffer[i];
			}
			else
			{
				for (int i = _Head; i < _Buffer.Length; ++i)
					yield return _Buffer[i];
				for (int i = 0; i < _Tail; ++i)
					yield return _Buffer[i];
			}
		}

		System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
		{
			return this.GetEnumerator();
		}
		#endregion
	}
