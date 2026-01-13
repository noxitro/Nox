using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Runtime.InteropServices.ComTypes;
using System.IO;
using System.Runtime.Versioning;

namespace Nox
{
	public static partial class Util
	{
		// 既存: Assert / VisualStudioAttachToProcess ...

		private sealed record DteCacheEntry(EnvDTE80.DTE2 Dte, string SolutionFullPath, long ExpiresAtTick);

		private static readonly object s_dteCacheLock = new();
		private static DteCacheEntry? s_dteCache;
		private const int DteCacheTtlMs = 2_000;

		public static void VisualStudioAttachToProcess(int pid, string solutionFullPath)
		{
			if (pid <= 0)
			{
				throw new ArgumentOutOfRangeException(nameof(pid));
			}

			if (string.IsNullOrWhiteSpace(solutionFullPath))
			{
				throw new ArgumentException("ソリューションパスが空です。", nameof(solutionFullPath));
			}

			var expected = Path.GetFullPath(solutionFullPath);

			// Fast path: 目的 PID を保持している DTE があれば、Solution.FullName を触らずに即 Attach。
			var dte = TryFindRunningDte2ByPid(pid);
			if (dte is null)
			{
				dte = FindRunningDte2BySolutionPathCached(expected);
			}

			if (dte is null)
			{
				throw new InvalidOperationException($"指定ソリューションを開いている Visual Studio が見つかりません: {solutionFullPath}");
			}

			EnvDTE.Process? target = null;

			foreach (EnvDTE.Process p in dte.Debugger.LocalProcesses)
			{
				if (p.ProcessID == pid)
				{
					target = p;
					break;
				}
			}

			if (target is null)
			{
				throw new InvalidOperationException($"PID={pid} のプロセスが Visual Studio の LocalProcesses に見つかりません。");
			}

			target.Attach();
		}

		private static EnvDTE80.DTE2? FindRunningDte2BySolutionPathCached(string expectedFullPath)
		{
			var now = Environment.TickCount64;
			lock (s_dteCacheLock)
			{
				if (s_dteCache is not null && s_dteCache.ExpiresAtTick >= now)
				{
					if (string.Equals(s_dteCache.SolutionFullPath, expectedFullPath, StringComparison.OrdinalIgnoreCase))
					{
						try
						{
							_ = s_dteCache.Dte.Debugger; // COM 生存確認
							return s_dteCache.Dte;
						}
						catch
						{
							s_dteCache = null;
						}
					}
				}
			}

			var dte = FindRunningDte2BySolutionPath(expectedFullPath);
			if (dte is null)
			{
				return null;
			}

			lock (s_dteCacheLock)
			{
				s_dteCache = new(dte, expectedFullPath, Environment.TickCount64 + DteCacheTtlMs);
			}

			return dte;
		}
		private static EnvDTE80.DTE2? TryFindRunningDte2ByPid(int pid)
		{
			IRunningObjectTable? rot = null;
			IEnumMoniker? enumMoniker = null;

			try
			{
				Marshal.ThrowExceptionForHR(GetRunningObjectTable(0, out rot));
				if (rot is null)
				{
					return null;
				}

				rot.EnumRunning(out enumMoniker);
				if (enumMoniker is null)
				{
					return null;
				}

				IBindCtx? bindCtx = null;
				Marshal.ThrowExceptionForHR(CreateBindCtx(0, out bindCtx));
				if (bindCtx is null)
				{
					return null;
				}

				var fetched = IntPtr.Zero;
				var monikers = new IMoniker[1];

				while (enumMoniker.Next(1, monikers, fetched) == 0)
				{
					var moniker = monikers[0];

					string? displayName = null;
					try
					{
						moniker.GetDisplayName(bindCtx, null, out displayName);
					}
					catch
					{
						continue;
					}

					if (string.IsNullOrEmpty(displayName))
					{
						continue;
					}

					if (displayName.IndexOf("VisualStudio.DTE", StringComparison.OrdinalIgnoreCase) < 0)
					{
						continue;
					}

					object? comObject = null;
					try
					{
						rot.GetObject(moniker, out comObject);
					}
					catch
					{
						continue;
					}

					if (comObject is not EnvDTE80.DTE2 dte)
					{
						continue;
					}

					try
					{
						foreach (EnvDTE.Process p in dte.Debugger.LocalProcesses)
						{
							if (p.ProcessID == pid)
							{
								return dte;
							}
						}
					}
					catch
					{
						continue;
					}
				}

				return null;
			}
			finally
			{
				if (enumMoniker is not null)
				{
					Marshal.ReleaseComObject(enumMoniker);
				}

				if (rot is not null)
				{
					Marshal.ReleaseComObject(rot);
				}
			}
		}
		private static EnvDTE80.DTE2? FindRunningDte2BySolutionPath(string solutionFullPath)
		{
			var expected = Path.GetFullPath(solutionFullPath);

			IRunningObjectTable? rot = null;
			IEnumMoniker? enumMoniker = null;

			try
			{
				Marshal.ThrowExceptionForHR(GetRunningObjectTable(0, out rot));
				if (rot is null)
				{
					return null;
				}

				rot.EnumRunning(out enumMoniker);
				if (enumMoniker is null)
				{
					return null;
				}

				IBindCtx? bindCtx = null;
				Marshal.ThrowExceptionForHR(CreateBindCtx(0, out bindCtx));
				if (bindCtx is null)
				{
					return null;
				}

				var fetched = IntPtr.Zero;
				var monikers = new IMoniker[1];

				while (enumMoniker.Next(1, monikers, fetched) == 0)
				{
					var moniker = monikers[0];

					string? displayName = null;
					try
					{
						moniker.GetDisplayName(bindCtx, null, out displayName);
					}
					catch
					{
						continue;
					}

					if (string.IsNullOrEmpty(displayName))
					{
						continue;
					}

					if (displayName.IndexOf("VisualStudio.DTE", StringComparison.OrdinalIgnoreCase) < 0)
					{
						continue;
					}

					object? comObject = null;
					try
					{
						rot.GetObject(moniker, out comObject);
					}
					catch
					{
						continue;
					}

					var dte = comObject as EnvDTE80.DTE2;
					if (dte is null)
					{
						continue;
					}

					string? openedSolution = null;
					try
					{
						openedSolution = dte.Solution?.FullName;
					}
					catch
					{
						continue;
					}

					if (string.IsNullOrWhiteSpace(openedSolution))
					{
						continue;
					}

					string normalizedOpened;
					try
					{
						normalizedOpened = Path.GetFullPath(openedSolution);
					}
					catch
					{
						continue;
					}

					if (string.Equals(normalizedOpened, expected, StringComparison.OrdinalIgnoreCase))
					{
						return dte;
					}
				}

				return null;
			}
			finally
			{
				if (enumMoniker is not null)
				{
					Marshal.ReleaseComObject(enumMoniker);
				}

				if (rot is not null)
				{
					Marshal.ReleaseComObject(rot);
				}
			}
		}

		[DllImport("ole32.dll")]
		private static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable? pprot);

		[DllImport("ole32.dll")]
		private static extern int CreateBindCtx(int reserved, out IBindCtx? ppbc);
	}
}
