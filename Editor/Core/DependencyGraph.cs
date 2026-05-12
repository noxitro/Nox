using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace Core
{
	public sealed record DependencyGraphNode(
		string Id,
		string Label,
		string Subtitle,
		string Phase,
		int Layer);

	public sealed record DependencyGraphEdge(
		string FromId,
		string ToId,
		string Label);

	public sealed class DependencyGraphSnapshot
	{
		public DependencyGraphSnapshot(string title, IReadOnlyList<DependencyGraphNode> nodes, IReadOnlyList<DependencyGraphEdge> edges)
		{
			Title = title;
			Nodes = nodes;
			Edges = edges;
		}

		public string Title { get; }
		public IReadOnlyList<DependencyGraphNode> Nodes { get; }
		public IReadOnlyList<DependencyGraphEdge> Edges { get; }
		public bool IsEmpty => Nodes.Count == 0;

		public static DependencyGraphSnapshot Empty(string title) => new(title, [], []);

		public static DependencyGraphSnapshot FromRuntimeText(string title, string text)
		{
			if (string.IsNullOrWhiteSpace(text))
			{
				return Empty(title);
			}

			List<DependencyGraphNode> nodes = new();
			List<DependencyGraphEdge> edges = new();
			foreach (string rawLine in text.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
			{
				string[] parts = rawLine.Split('|');
				if (parts.Length == 0)
				{
					continue;
				}

				if (string.Equals(parts[0], "NODE", StringComparison.Ordinal) && parts.Length >= 6)
				{
					int layer = int.TryParse(parts[5], NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsedLayer) ? parsedLayer : 0;
					nodes.Add(new DependencyGraphNode(parts[1], parts[2], parts[3], parts[4], layer));
				}
				else if (string.Equals(parts[0], "EDGE", StringComparison.Ordinal) && parts.Length >= 4)
				{
					edges.Add(new DependencyGraphEdge(parts[1], parts[2], parts[3]));
				}
			}

			return new DependencyGraphSnapshot(title, nodes, edges);
		}
	}

	public static class DependencyGraphBuilder
	{
		public static DependencyGraphSnapshot CreateEditorEngineSystemGraph(StudioManager studioManager)
		{
			PhaseRegister[] phaseRegisters = studioManager.GetEngineSystemList()
				.SelectMany(static system => system.GetPhaseRegisterList())
				.ToArray();

			List<DependencyGraphNode> nodes = new(phaseRegisters.Length);
			List<DependencyGraphEdge> edges = new();
			Dictionary<string, int> layerByPhaseName = CalculateLayers(phaseRegisters);

			foreach (PhaseRegister register in phaseRegisters)
			{
				string id = CreateEditorNodeId(register.Phase);
				nodes.Add(new DependencyGraphNode(
					id,
					register.Phase.Name,
					register.Phase.PhaseType.ToString(),
					register.Phase.PhaseType.ToString(),
					layerByPhaseName.GetValueOrDefault(register.Phase.Name, 0)));

				foreach (SystemPhase dependency in register.Dependencies)
				{
					edges.Add(new DependencyGraphEdge(CreateEditorNodeId(dependency), id, "depends"));
				}

				foreach (SystemPhase dependent in register.Dependents)
				{
					edges.Add(new DependencyGraphEdge(id, CreateEditorNodeId(dependent), "before"));
				}
			}

			HashSet<string> knownNodeIds = nodes.Select(static node => node.Id).ToHashSet(StringComparer.Ordinal);
			edges.RemoveAll(edge => knownNodeIds.Contains(edge.FromId) == false || knownNodeIds.Contains(edge.ToId) == false);
			return new DependencyGraphSnapshot("Editor Engine System Phases", nodes, edges);
		}

		private static Dictionary<string, int> CalculateLayers(IReadOnlyList<PhaseRegister> phaseRegisters)
		{
			Dictionary<string, PhaseRegister> registerByName = phaseRegisters
				.GroupBy(static register => register.Phase.Name, StringComparer.Ordinal)
				.ToDictionary(static group => group.Key, static group => group.First(), StringComparer.Ordinal);
			Dictionary<string, int> layerByName = new(StringComparer.Ordinal);
			HashSet<string> visiting = new(StringComparer.Ordinal);

			int Visit(string phaseName)
			{
				if (layerByName.TryGetValue(phaseName, out int cachedLayer))
				{
					return cachedLayer;
				}

				if (visiting.Add(phaseName) == false || registerByName.TryGetValue(phaseName, out PhaseRegister register) == false)
				{
					return 0;
				}

				int layer = 0;
				foreach (SystemPhase dependency in register.Dependencies)
				{
					layer = Math.Max(layer, Visit(dependency.Name) + 1);
				}

				visiting.Remove(phaseName);
				layerByName[phaseName] = layer;
				return layer;
			}

			foreach (PhaseRegister register in phaseRegisters)
			{
				Visit(register.Phase.Name);
			}

			return layerByName;
		}

		private static string CreateEditorNodeId(SystemPhase phase)
		{
			return $"editor:{phase.PhaseType}:{phase.Name}";
		}
	}
}
