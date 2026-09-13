// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;

namespace Core.UI;

	public enum CoreDiagnosticsViewKind
	{
		EngineSystemGraph,
		RemoteInstances,
		MemoryProfiler,
	}

	public static class CoreDiagnosticsViewService
	{
		private static Action<CoreDiagnosticsViewKind>? _Show;

		public static void Register(Action<CoreDiagnosticsViewKind> show)
		{
			_Show = show;
		}

		public static void Unregister(Action<CoreDiagnosticsViewKind> show)
		{
			if (_Show == show)
			{
				_Show = null;
			}
		}

		public static void Show(CoreDiagnosticsViewKind kind)
		{
			_Show?.Invoke(kind);
		}
	}
