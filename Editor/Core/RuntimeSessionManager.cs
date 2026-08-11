using System;
using System.Collections.Generic;

namespace Core;

	public sealed class RuntimeSessionManager : IDisposable
	{
		#region 非公開フィールド
		private readonly Workspace _Workspace;
		private readonly List<RuntimeSession> _SessionList = new();
		private RuntimeSession? _ActiveSession;
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public IReadOnlyList<RuntimeSession> Sessions => _SessionList;
		public RuntimeSession? ActiveSession
		{
			get => _ActiveSession;
			set
			{
				if (value != null)
				{
					Nox.Util.Assert(_SessionList.Contains(value), "ActiveSession must be owned by this manager.");
				}

				_ActiveSession = value;
			}
		}
		#endregion

		public RuntimeSessionManager(Workspace workspace)
		{
			_Workspace = workspace;
		}

		public RuntimeSession CreateSession(RuntimeSessionKind kind)
		{
			Nox.Util.Assert(_Disposed == false, "RuntimeSessionManager is disposed.");

			RuntimeSession session = new(_Workspace, kind);
			_SessionList.Add(session);
			_ActiveSession ??= session;
			return session;
		}

		public RuntimeSession GetOrCreateMainSession()
		{
			foreach (RuntimeSession session in _SessionList)
			{
				if (session.Kind == RuntimeSessionKind.Main)
				{
					_ActiveSession ??= session;
					return session;
				}
			}

			return CreateSession(RuntimeSessionKind.Main);
		}

		public RuntimeSession? FindMainSession()
		{
			foreach (RuntimeSession session in _SessionList)
			{
				if (session.Kind == RuntimeSessionKind.Main)
				{
					return session;
				}
			}

			return null;
		}

		public RuntimeSession GetActiveOrMainSession()
		{
			return _ActiveSession ?? GetOrCreateMainSession();
		}

		public bool RemoveSession(RuntimeSession session)
		{
			if (_SessionList.Remove(session) == false)
			{
				return false;
			}

			session.Dispose();
			if (_ActiveSession == session)
			{
				_ActiveSession = null;
			}
			return true;
		}

		public void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			for (int i = _SessionList.Count - 1; i >= 0; --i)
			{
				_SessionList[i].Dispose();
			}

			_SessionList.Clear();
			_ActiveSession = null;
			_Disposed = true;
		}
	}
