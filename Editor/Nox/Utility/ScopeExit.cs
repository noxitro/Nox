// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Utility;

	/// <summary>
	/// ゼロアロケーション版 ScopeExit。
	/// delegate* を使うため、アクションは静的メソッドである必要があります。
	/// </summary>
	public unsafe ref struct ScopeExit<T> : IDisposable
	{
		public ScopeExit(T state, delegate* managed<T, void> action)
		{
			_State = state;
			_Action = action;
		}

		readonly void IDisposable.Dispose() => _Action(_State);

		private readonly T _State;
		private readonly delegate* managed<T, void> _Action;
	}
