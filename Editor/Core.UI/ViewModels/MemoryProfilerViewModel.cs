// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Windows.Threading;

namespace Core.UI.ViewModels;

	public sealed class MemoryAllocationViewModel : NoxUI.ViewModelBase
	{
		public ushort ProfileHandle { get; init; }
		public uint Size { get; init; }
		public uint AlignSize { get; init; }
		public required string InstanceType { get; init; }
		public required string SegmentType { get; init; }
		public required string CallStack { get; init; }
	}

	public sealed class MemoryProfilerViewModel : NoxUI.ViewModelBase, IDisposable
	{
		private readonly DispatcherTimer _RefreshTimer;
		private readonly object _SnapshotPendingLock = new();
		private bool _SnapshotPending;
		private bool _Disposed;

		public ObservableCollection<MemoryAllocationViewModel> Allocations { get; } = new();

		public string StatusText
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public string SummaryText
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public bool AutoRefresh
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					if (value)
					{
						_RefreshTimer.Start();
					}
					else
					{
						_RefreshTimer.Stop();
					}
				}
			}
		} = true;

		public NoxUI.ViewModelCommand RefreshCommand => field ??= new(Refresh);

		public MemoryProfilerViewModel()
		{
			_RefreshTimer = new DispatcherTimer
			{
				Interval = TimeSpan.FromMilliseconds(1000),
			};
			_RefreshTimer.Tick += (_, _) => Refresh();
			_RefreshTimer.Start();
			Refresh();
		}

		private void Refresh()
		{
			if (BeginSnapshotRequest() == false)
			{
				return;
			}

			Core.RuntimeSession session = Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession();
			if (session.RemoteClient.IsConnected == false)
			{
				EndSnapshotRequest();
				StatusText = "Runtime is not connected.";
				SummaryText = string.Empty;
				Allocations.Clear();
				return;
			}

			StatusText = "Requesting memory profiler snapshot...";
			session.RemoteClient.SendQuery(new Core.RuntimeRemote.GetMemoryProfilerSnapshotQuery(), response =>
			{
				EndSnapshotRequest();
				if (_Disposed)
				{
					return;
				}

				Core.RuntimeRemote.MemoryProfilerSnapshotResponse snapshotResponse = Nox.Util.Cast<Core.RuntimeRemote.MemoryProfilerSnapshotResponse>(response);
				Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
				void Apply() => ApplySnapshot(snapshotResponse.SnapshotText);

				if (NoxUI.DispatcherHelper.IsShuttingDown(dispatcher))
				{
					return;
				}

				if (dispatcher!.CheckAccess() == false)
				{
					NoxUI.DispatcherHelper.TryBeginInvoke(dispatcher, Apply);
				}
				else
				{
					Apply();
				}
			});
		}

		private bool BeginSnapshotRequest()
		{
			lock (_SnapshotPendingLock)
			{
				if (_SnapshotPending)
				{
					return false;
				}

				_SnapshotPending = true;
				return true;
			}
		}

		private void EndSnapshotRequest()
		{
			lock (_SnapshotPendingLock)
			{
				_SnapshotPending = false;
			}
		}

		private void ApplySnapshot(string snapshotText)
		{
			Allocations.Clear();
			uint totalBytes = 0;
			uint allocationCount = 0;
			bool enabled = false;
			uint truncatedCount = 0;

			foreach (string line in snapshotText.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
			{
				string[] parts = line.Split('|');
				if (parts.Length == 0)
				{
					continue;
				}

				if (string.Equals(parts[0], "SUMMARY", StringComparison.Ordinal) && parts.Length >= 5)
				{
					enabled = parts[1] == "1";
					allocationCount = ParseUInt32(parts[2]);
					totalBytes = ParseUInt32(parts[3]);
					truncatedCount = ParseUInt32(parts[4]);
				}
				else if (string.Equals(parts[0], "ALLOC", StringComparison.Ordinal) && parts.Length >= 7)
				{
					Allocations.Add(new MemoryAllocationViewModel
					{
						ProfileHandle = (ushort)ParseUInt32(parts[1]),
						Size = ParseUInt32(parts[2]),
						AlignSize = ParseUInt32(parts[3]),
						InstanceType = parts[4],
						SegmentType = parts[5],
						CallStack = parts[6],
					});
				}
			}

			SummaryText =
				$"Profiler={(enabled ? "Enabled" : "Disabled")} / " +
				$"Allocations={allocationCount.ToString(CultureInfo.InvariantCulture)} / " +
				$"Total={FormatBytes(totalBytes)} / " +
				$"Displayed={Allocations.Count.ToString(CultureInfo.InvariantCulture)} / " +
				$"Truncated={truncatedCount.ToString(CultureInfo.InvariantCulture)}";
			StatusText = "Memory profiler snapshot updated.";
		}

		private static uint ParseUInt32(string value)
		{
			return uint.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out uint parsed) ? parsed : 0;
		}

		private static string FormatBytes(uint bytes)
		{
			const double kib = 1024.0;
			const double mib = kib * 1024.0;
			if (bytes >= mib)
			{
				return (bytes / mib).ToString("F2", CultureInfo.InvariantCulture) + " MiB";
			}
			if (bytes >= kib)
			{
				return (bytes / kib).ToString("F2", CultureInfo.InvariantCulture) + " KiB";
			}
			return bytes.ToString(CultureInfo.InvariantCulture) + " B";
		}

		public override void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			_RefreshTimer.Stop();
			_Disposed = true;
		}
	}
