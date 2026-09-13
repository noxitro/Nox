// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Nox;

	public abstract class DelegateHandler : System.IDisposable
	{
		#region 公開メソッド

		public DelegateHandler()
		{
		}

		public void Dispose()
		{
			//	リークチェック
			if (_HandleList.Count > 0)
			{
				foreach (var handle in _HandleList)
				{
					Nox.LogTrace.ErrorLine<Nox.LogId.Kernel>($"DelegateHandle is leaked. Delegate={handle}");
				}
			}
		}

		public Nox.DelegateHandle Add(System.Delegate d)
		{
			_Delegates = System.Delegate.Combine(_Delegates, d);

			var handle = new Nox.DelegateHandle(d, this);
			_HandleList.Add(handle);
			return handle;
		}

		public void ReleaseHandle(System.Delegate d, in Nox.DelegateHandle handle)
		{
			_Delegates = System.Delegate.Remove(_Delegates, d);
			_HandleList.Remove(handle);
		}
		#endregion

		#region 非公開フィールド
		protected System.Delegate? _Delegates;
		private readonly List<DelegateHandle> _HandleList = new();
		#endregion
	}

	public class ActionHandler : DelegateHandler
	{
		public void Invoke()
		{
			if (_Delegates != null)
			{
				System.Runtime.CompilerServices.Unsafe.As<Action>(_Delegates).Invoke();
			}
		}
	}

	public class ActionHandler<T0> : DelegateHandler
	{
		public void Invoke(T0 v0)
		{
			if (_Delegates != null)
			{
				System.Runtime.CompilerServices.Unsafe.As<Action<T0>>(_Delegates).Invoke(v0);
			}
		}
	}

	public struct DelegateHandle : System.IDisposable, System.IEquatable<DelegateHandle> 
	{
		#region 非公開フィールド
		private System.Delegate? _Delegate;
		private Nox.DelegateHandler? _Owner;
		#endregion

		#region 公開メソッド
		public DelegateHandle(System.Delegate d, Nox.DelegateHandler owner)
		{
			_Delegate = d;
			_Owner  = owner;
		}

		public void Dispose()
		{
			if (_Owner != null && _Delegate != null)
			{
				_Owner.ReleaseHandle(_Delegate, this);
				_Owner=null;
			}
		}

		bool IEquatable<DelegateHandle>.Equals(DelegateHandle other)
		{
			return _Delegate == other._Delegate;
		}

		public override int GetHashCode()
		{
			if (_Delegate == null)
				return 0;
			return _Delegate.GetHashCode();
		}
		#endregion
	}

	//public class AppTest
	//{ 
	//	//	delegate handler

	//	public static DelegateHandle registerFunc(Action f)
	//	{
	//		return handler.Add(f);
	//		// or 
	//		// handler += f;
	//	}
	//}

	//public class UnitObjTest
	//{
	//	DelegateHandle handle;

	//	public void test()
	//	{
	//		handle = AppTest.registerFunc(() => { });
	//	}
	//}
