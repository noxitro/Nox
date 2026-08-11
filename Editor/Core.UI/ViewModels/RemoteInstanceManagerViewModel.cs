using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Windows.Threading;

namespace Core.UI.ViewModels;

	public sealed class RemoteInstanceItemViewModel : NoxUI.ViewModelBase
	{
		public required string Session { get; init; }
		public required string Source { get; init; }
		public long RemoteInstanceId { get; init; }
		public required string Owner { get; init; }
		public required string TypeName { get; init; }
		public required string RuntimeFqn { get; init; }
		public int VariableCount { get; init; }
		public int DirtyVariableCount { get; init; }
		public required string Status { get; init; }
	}

	public sealed class RemoteInstanceManagerViewModel : NoxUI.ViewModelBase, IDisposable
	{
		private readonly DispatcherTimer _RefreshTimer;
		private readonly object _RuntimeSnapshotPendingLock = new();
		private readonly HashSet<int> _RuntimeSnapshotPendingSessionIds = new();
		private bool _Disposed;

		public ObservableCollection<RemoteInstanceItemViewModel> Instances { get; } = new();

		public string StatusText
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

		public RemoteInstanceManagerViewModel()
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
			Instances.Clear();
			Core.RuntimeSessionManager sessionManager = Core.StudioManager.Instance.Workspace.RuntimeSessions;
			foreach (Core.RuntimeSession session in sessionManager.Sessions)
			{
				AddEditorClientInstances(session);
				RequestRuntimeServerInstances(session);
			}

			if (sessionManager.Sessions.Count == 0)
			{
				StatusText = "No runtime sessions.";
			}
			else
			{
				StatusText = $"Instances: {Instances.Count.ToString(CultureInfo.InvariantCulture)}";
			}
		}

		private void AddEditorClientInstances(Core.RuntimeSession session)
		{
			foreach (Core.Net.RuntimeRemoteClient.RemoteInstanceSnapshot snapshot in session.RemoteClient.GetRemoteInstanceSnapshots())
			{
				Instances.Add(new RemoteInstanceItemViewModel
				{
					Session = FormatSession(session),
					Source = "Editor Client",
					RemoteInstanceId = snapshot.RemoteInstanceId,
					Owner = snapshot.Owner,
					TypeName = snapshot.TypeName,
					RuntimeFqn = snapshot.RuntimeFqn,
					VariableCount = snapshot.VariableCount,
					DirtyVariableCount = snapshot.DirtyVariableCount,
					Status = session.RemoteClient.State.ToString(),
				});
			}
		}

		private void RequestRuntimeServerInstances(Core.RuntimeSession session)
		{
			if (session.RemoteClient.IsConnected == false || BeginRuntimeSnapshotRequest(session.Id) == false)
			{
				return;
			}

			session.RemoteClient.SendQuery(new Core.RuntimeRemote.GetRemoteInstanceSnapshotQuery(), response =>
			{
				EndRuntimeSnapshotRequest(session.Id);
				if (_Disposed)
				{
					return;
				}

				Core.RuntimeRemote.RemoteInstanceSnapshotResponse snapshotResponse = Nox.Util.Cast<Core.RuntimeRemote.RemoteInstanceSnapshotResponse>(response);
				Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
				void Apply()
				{
					AppendRuntimeServerInstances(session, snapshotResponse.SnapshotText);
					StatusText = $"Instances: {Instances.Count.ToString(CultureInfo.InvariantCulture)}";
				}

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

		private bool BeginRuntimeSnapshotRequest(int sessionId)
		{
			lock (_RuntimeSnapshotPendingLock)
			{
				return _RuntimeSnapshotPendingSessionIds.Add(sessionId);
			}
		}

		private void EndRuntimeSnapshotRequest(int sessionId)
		{
			lock (_RuntimeSnapshotPendingLock)
			{
				_RuntimeSnapshotPendingSessionIds.Remove(sessionId);
			}
		}

		private void AppendRuntimeServerInstances(Core.RuntimeSession session, string snapshotText)
		{
			foreach (string line in snapshotText.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
			{
				string[] parts = line.Split('|');
				if (parts.Length < 5 || string.Equals(parts[0], "INSTANCE", StringComparison.Ordinal) == false)
				{
					continue;
				}

				long id = long.TryParse(parts[1], NumberStyles.Integer, CultureInfo.InvariantCulture, out long parsedId) ? parsedId : 0;
				Instances.Add(new RemoteInstanceItemViewModel
				{
					Session = FormatSession(session),
					Source = "Runtime Server",
					RemoteInstanceId = id,
					Owner = id >= 0 ? "Editor" : "Runtime",
					TypeName = parts[3],
					RuntimeFqn = parts[2],
					VariableCount = 0,
					DirtyVariableCount = 0,
					Status = parts[4],
				});
			}
		}

		private static string FormatSession(Core.RuntimeSession session)
		{
			return $"{session.Kind} #{session.Id.ToString(CultureInfo.InvariantCulture)}";
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
