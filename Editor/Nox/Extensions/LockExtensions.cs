// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Extensions;

	public static class LockExtensions
	{
		public readonly ref struct ReadLockScope : IDisposable
		{
			private readonly System.Threading.ReaderWriterLockSlim _Lock;
			public ReadLockScope(System.Threading.ReaderWriterLockSlim lock_)
			{
				_Lock = lock_;
				_Lock.EnterReadLock();
			}
			readonly void IDisposable.Dispose() => _Lock.ExitReadLock();
		}

		public readonly ref struct WriteLockScope : IDisposable
		{
			private readonly System.Threading.ReaderWriterLockSlim _Lock;
			public WriteLockScope(System.Threading.ReaderWriterLockSlim lock_)
			{
				_Lock = lock_;
				_Lock.EnterWriteLock();
			}
			readonly void IDisposable.Dispose() => _Lock.ExitWriteLock();
		}

		public static ReadLockScope ReadLock(this System.Threading.ReaderWriterLockSlim self)
				=> new ReadLockScope(self);

		public static WriteLockScope WriteLock(this System.Threading.ReaderWriterLockSlim self)
			=> new WriteLockScope(self);
	}
