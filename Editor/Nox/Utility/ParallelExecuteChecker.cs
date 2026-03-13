using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Utility
{
	public ref struct ParallelExecuteCheckScope : IDisposable
	{
		public ParallelExecuteCheckScope(ref ParallelExecuteChecker checker)
		{
			_Checker = ref checker;
			_Checker.Enter();
		}
		void IDisposable.Dispose()
		{
			_Checker.Exit();
		}
		private readonly ref ParallelExecuteChecker _Checker;
	}

	public struct ParallelExecuteChecker
	{
		public ParallelExecuteChecker() { }

		public void Enter()
		{
			if (System.Threading.Interlocked.Increment(ref _Counter) > 1)
			{
				System.Threading.Interlocked.Decrement(ref _Counter);
				Nox.LogTrace.ErrorLine<Nox.LogId.Kernel>($"並列実行を検知しました:\n{Environment.StackTrace}");
			}
		}

		public void Exit()
		{
			System.Threading.Interlocked.Decrement(ref _Counter);
		}

		private int _Counter = 0;
	}

	public ref struct ReadParallelExecuteCheckScope : System.IDisposable
	{
		public ReadParallelExecuteCheckScope(ref RWParallelExecuteChecker checker)
		{
			_Checker = ref checker;
			_Checker.EnterRead();
		}
		readonly void IDisposable.Dispose()
		{
			_Checker.ExitRead();
		}
		private readonly ref RWParallelExecuteChecker _Checker;
	}

	public ref struct WriteParallelExecuteCheckScope : System.IDisposable
	{
		public WriteParallelExecuteCheckScope(ref RWParallelExecuteChecker checker)
		{
			_Checker = ref checker;
			_Checker.EnterWrite();
		}
		readonly void IDisposable.Dispose()
		{
			_Checker.ExitWrite();
		}
		private readonly ref RWParallelExecuteChecker _Checker;
	}

	public struct RWParallelExecuteChecker
	{
		public RWParallelExecuteChecker() { }
		public void EnterRead()
		{
			System.Threading.Interlocked.Increment(ref _ReadCounter);
		}
		public void ExitRead()
		{
			System.Threading.Interlocked.Decrement(ref _ReadCounter);
		}
		public void EnterWrite()
		{
			if (System.Threading.Interlocked.Increment(ref _WriteCounter) > 1 || _ReadCounter > 0)
			{
				System.Threading.Interlocked.Decrement(ref _WriteCounter);
				Nox.LogTrace.ErrorLine<Nox.LogId.Kernel>($"並列実行を検知しました:\n{Environment.StackTrace}");
			}
		}
		public void ExitWrite()
		{
			System.Threading.Interlocked.Decrement(ref _WriteCounter);
		}
		private int _ReadCounter = 0;
		private int _WriteCounter = 0;
	}

	public static class ParallelExecuteCheckerExtensions
	{
		public static ParallelExecuteCheckScope EnterScope(ref this ParallelExecuteChecker checker)
		{
			return new ParallelExecuteCheckScope(ref checker);
		}

		public static ReadParallelExecuteCheckScope EnterReadScope(ref this RWParallelExecuteChecker checker)
		{
			return new ReadParallelExecuteCheckScope(ref checker);
		}

		public static WriteParallelExecuteCheckScope EnterWriteScope(ref this RWParallelExecuteChecker checker)
		{
			return new WriteParallelExecuteCheckScope(ref checker);
		}
	}
}
