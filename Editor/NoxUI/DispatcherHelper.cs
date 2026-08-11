using System;
using System.Windows.Threading;

namespace NoxUI;

	public static class DispatcherHelper
	{
		public static bool IsShuttingDown(Dispatcher? dispatcher)
		{
			return dispatcher == null || dispatcher.HasShutdownStarted || dispatcher.HasShutdownFinished;
		}

		public static bool TryBeginInvoke(Dispatcher? dispatcher, Action action, DispatcherPriority priority = DispatcherPriority.Normal)
		{
			ArgumentNullException.ThrowIfNull(action);

			if (IsShuttingDown(dispatcher))
			{
				return false;
			}

			Dispatcher liveDispatcher = dispatcher!;
			try
			{
				liveDispatcher.BeginInvoke(action, priority);
				return true;
			}
			catch (InvalidOperationException) when (liveDispatcher.HasShutdownStarted || liveDispatcher.HasShutdownFinished)
			{
				return false;
			}
		}
	}
