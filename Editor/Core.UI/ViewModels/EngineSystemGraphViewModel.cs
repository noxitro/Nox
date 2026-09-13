// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.ObjectModel;
using System.Windows.Threading;

namespace Core.UI.ViewModels;

	public sealed class EngineSystemGraphViewModel : NoxUI.ViewModelBase
	{
		private bool _RuntimeGraphPending;

		public ObservableCollection<PropertyInspectorFieldViewModel> GraphFields { get; } = new();

		public string StatusText
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public NoxUI.ViewModelCommand RefreshCommand => field ??= new(Refresh);

		public EngineSystemGraphViewModel()
		{
			Refresh();
		}

		private void Refresh()
		{
			GraphFields.Clear();
			AddGraph("Editor EngineSystem", Core.DependencyGraphBuilder.CreateEditorEngineSystemGraph(Core.StudioManager.Instance));
			RequestRuntimeGraph();
		}

		private void RequestRuntimeGraph()
		{
			Core.RuntimeSession session = Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession();
			if (session.RemoteClient.IsConnected == false)
			{
				StatusText = "Runtime is not connected. Showing editor EngineSystem graph only.";
				return;
			}
			if (_RuntimeGraphPending)
			{
				return;
			}

			_RuntimeGraphPending = true;
			StatusText = "Requesting runtime EngineSystem graph...";
			session.RemoteClient.SendQuery(new Core.RuntimeRemote.GetRuntimeDependencyGraphQuery(), response =>
			{
				_RuntimeGraphPending = false;
				Core.RuntimeRemote.RuntimeDependencyGraphResponse graphResponse = Nox.Util.Cast<Core.RuntimeRemote.RuntimeDependencyGraphResponse>(response);
				Core.DependencyGraphSnapshot snapshot = Core.DependencyGraphSnapshot.FromRuntimeText("Runtime EngineSystem Phases", graphResponse.GraphText);

				Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
				void Apply()
				{
					AddGraph("Runtime EngineSystem", snapshot);
					StatusText = $"Graphs: {GraphFields.Count}";
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

		private void AddGraph(string name, Core.DependencyGraphSnapshot snapshot)
		{
			PropertyInspectorGraphFieldViewModel graph = new()
			{
				Name = name,
				DisplayName = name,
				Description = snapshot.Title,
				Category = "EngineSystem",
				TypeName = nameof(Core.DependencyGraphSnapshot),
				IsReadOnly = true,
			};
			graph.SetGraph(snapshot);
			GraphFields.Add(graph);
			StatusText = $"Graphs: {GraphFields.Count}";
		}
	}
