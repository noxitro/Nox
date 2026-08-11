// Copyright (c) 2026 NOX ENGINE All rights reserved.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using System.Windows.Input;

namespace Core.UI.ViewModels;

	public enum PropertyInspectorFieldKind
	{
		ReadOnly,
		Text,
		Boolean,
		Enum,
		Vector3,
		Action,
		Graph,
	}

	public abstract class PropertyInspectorFieldViewModel : NoxUI.ViewModelBase
	{
		public string Name
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;

		public string DisplayName
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;

		public string Description
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(ToolTipText));
				}
			}
		} = string.Empty;

		public string Category
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;

		public string TypeName
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(ToolTipText));
				}
			}
		} = string.Empty;

		public string ToolTipText
		{
			get
			{
				if (string.IsNullOrWhiteSpace(TypeName))
				{
					return Description;
				}

				if (string.IsNullOrWhiteSpace(Description))
				{
					return TypeName;
				}

				return TypeName + Environment.NewLine + Description;
			}
		}

		public string ErrorText
		{
			get => field;
			private set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(HasError));
				}
			}
		} = string.Empty;

		public bool HasError => string.IsNullOrWhiteSpace(ErrorText) == false;

		public bool IsReadOnly
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(IsEditable));
				}
			}
		}

		public bool IsEditable => IsReadOnly == false;

		public abstract PropertyInspectorFieldKind Kind { get; }

		public void SetError(string message)
		{
			ErrorText = message;
		}
	}

	public sealed class PropertyInspectorReadOnlyFieldViewModel : PropertyInspectorFieldViewModel
	{
		public override PropertyInspectorFieldKind Kind => PropertyInspectorFieldKind.ReadOnly;

		public string Value
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;
	}

	public sealed class PropertyInspectorTextFieldViewModel : PropertyInspectorFieldViewModel
	{
		private readonly Action<string>? _Commit;
		private bool _SuppressCommit;

		public PropertyInspectorTextFieldViewModel(Action<string>? commit = null)
		{
			_Commit = commit;
		}

		public override PropertyInspectorFieldKind Kind => PropertyInspectorFieldKind.Text;

		public bool IsMultiline
		{
			get => field;
			set => SetProperty(ref field, value);
		}

		public string Value
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					if (_SuppressCommit == false)
					{
						_Commit?.Invoke(value);
					}
				}
			}
		} = string.Empty;

		public void SetInitialValue(string value)
		{
			_SuppressCommit = true;
			Value = value;
			_SuppressCommit = false;
		}
	}

	public sealed class PropertyInspectorBooleanFieldViewModel : PropertyInspectorFieldViewModel
	{
		private readonly Action<bool>? _Commit;
		private bool _SuppressCommit;

		public PropertyInspectorBooleanFieldViewModel(Action<bool>? commit = null)
		{
			_Commit = commit;
		}

		public override PropertyInspectorFieldKind Kind => PropertyInspectorFieldKind.Boolean;

		public bool Value
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					if (_SuppressCommit == false)
					{
						_Commit?.Invoke(value);
					}
				}
			}
		}

		public void SetInitialValue(bool value)
		{
			_SuppressCommit = true;
			Value = value;
			_SuppressCommit = false;
		}
	}

	public sealed class PropertyInspectorEnumFieldViewModel : PropertyInspectorFieldViewModel
	{
		private readonly Action<string>? _Commit;
		private bool _SuppressCommit;

		public PropertyInspectorEnumFieldViewModel(IEnumerable<string> values, Action<string>? commit = null)
		{
			Values = values.ToArray();
			_Commit = commit;
		}

		public override PropertyInspectorFieldKind Kind => PropertyInspectorFieldKind.Enum;
		public IReadOnlyList<string> Values { get; }

		public string SelectedValue
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					if (_SuppressCommit == false)
					{
						_Commit?.Invoke(value);
					}
				}
			}
		} = string.Empty;

		public void SetInitialValue(string value)
		{
			_SuppressCommit = true;
			SelectedValue = value;
			_SuppressCommit = false;
		}
	}

	public sealed class PropertyInspectorVector3FieldViewModel : PropertyInspectorFieldViewModel
	{
		private readonly Action<double, double, double>? _Commit;
		private bool _SuppressCommit;

		public PropertyInspectorVector3FieldViewModel(Action<double, double, double>? commit = null)
		{
			_Commit = commit;
		}

		public override PropertyInspectorFieldKind Kind => PropertyInspectorFieldKind.Vector3;

		public double X
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(XText));
					Commit();
				}
			}
		}

		public double Y
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(YText));
					Commit();
				}
			}
		}

		public double Z
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(ZText));
					Commit();
				}
			}
		}

		public string XText
		{
			get => X.ToString(CultureInfo.InvariantCulture);
			set
			{
				if (double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out double parsed))
				{
					X = parsed;
				}
			}
		}

		public string YText
		{
			get => Y.ToString(CultureInfo.InvariantCulture);
			set
			{
				if (double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out double parsed))
				{
					Y = parsed;
				}
			}
		}

		public string ZText
		{
			get => Z.ToString(CultureInfo.InvariantCulture);
			set
			{
				if (double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out double parsed))
				{
					Z = parsed;
				}
			}
		}

		public void SetInitialValue(double x, double y, double z)
		{
			_SuppressCommit = true;
			X = x;
			Y = y;
			Z = z;
			_SuppressCommit = false;
		}

		private void Commit()
		{
			if (_SuppressCommit == false)
			{
				_Commit?.Invoke(X, Y, Z);
			}
		}
	}

	public sealed class PropertyInspectorActionFieldViewModel : PropertyInspectorFieldViewModel
	{
		public PropertyInspectorActionFieldViewModel(Action execute)
		{
			ExecuteCommand = new NoxUI.ViewModelCommand(execute);
		}

		public override PropertyInspectorFieldKind Kind => PropertyInspectorFieldKind.Action;

		public ICommand ExecuteCommand { get; }
	}

	public sealed class PropertyInspectorGraphNodeViewModel : NoxUI.ViewModelBase
	{
		public required string Id { get; init; }
		public required string Label { get; init; }
		public required string Subtitle { get; init; }
		public required string Phase { get; init; }

		public double X
		{
			get => field;
			init => field = value;
		}

		public double Y
		{
			get => field;
			init => field = value;
		}
	}

	public sealed class PropertyInspectorGraphEdgeViewModel : NoxUI.ViewModelBase
	{
		public required string FromId { get; init; }
		public required string ToId { get; init; }
		public required string Label { get; init; }
		public double X1 { get; init; }
		public double Y1 { get; init; }
		public double X2 { get; init; }
		public double Y2 { get; init; }
	}

	public sealed class PropertyInspectorGraphFieldViewModel : PropertyInspectorFieldViewModel
	{
		private const double NodeWidth = 190.0;
		private const double NodeHeight = 58.0;
		private const double HorizontalGap = 58.0;
		private const double VerticalGap = 24.0;

		public override PropertyInspectorFieldKind Kind => PropertyInspectorFieldKind.Graph;

		public ObservableCollection<PropertyInspectorGraphNodeViewModel> Nodes { get; } = new();
		public ObservableCollection<PropertyInspectorGraphEdgeViewModel> Edges { get; } = new();

		public double CanvasWidth
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = 480.0;

		public double CanvasHeight
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = 120.0;

		public string Summary
		{
			get => field;
			private set => SetProperty(ref field, value);
		} = string.Empty;

		public void SetGraph(Core.DependencyGraphSnapshot snapshot)
		{
			Nodes.Clear();
			Edges.Clear();
			Summary = $"{snapshot.Nodes.Count.ToString(CultureInfo.InvariantCulture)} nodes / {snapshot.Edges.Count.ToString(CultureInfo.InvariantCulture)} edges";
			if (snapshot.IsEmpty)
			{
				CanvasWidth = 480.0;
				CanvasHeight = 96.0;
				return;
			}

			Dictionary<string, PropertyInspectorGraphNodeViewModel> nodeById = new(StringComparer.Ordinal);
			Dictionary<int, int> rowByLayer = new();
			foreach (Core.DependencyGraphNode node in snapshot.Nodes)
			{
				int row = rowByLayer.GetValueOrDefault(node.Layer, 0);
				rowByLayer[node.Layer] = row + 1;
				PropertyInspectorGraphNodeViewModel nodeViewModel = new()
				{
					Id = node.Id,
					Label = node.Label,
					Subtitle = string.IsNullOrWhiteSpace(node.Subtitle) ? node.Phase : node.Subtitle,
					Phase = node.Phase,
					X = node.Layer * (NodeWidth + HorizontalGap),
					Y = row * (NodeHeight + VerticalGap),
				};
				Nodes.Add(nodeViewModel);
				nodeById[node.Id] = nodeViewModel;
			}

			foreach (Core.DependencyGraphEdge edge in snapshot.Edges)
			{
				if (nodeById.TryGetValue(edge.FromId, out PropertyInspectorGraphNodeViewModel? from) == false ||
					nodeById.TryGetValue(edge.ToId, out PropertyInspectorGraphNodeViewModel? to) == false)
				{
					continue;
				}

				Edges.Add(new PropertyInspectorGraphEdgeViewModel
				{
					FromId = edge.FromId,
					ToId = edge.ToId,
					Label = edge.Label,
					X1 = from.X + NodeWidth,
					Y1 = from.Y + (NodeHeight * 0.5),
					X2 = to.X,
					Y2 = to.Y + (NodeHeight * 0.5),
				});
			}

			CanvasWidth = Math.Max(480.0, Nodes.Max(static node => node.X) + NodeWidth + 16.0);
			CanvasHeight = Math.Max(96.0, Nodes.Max(static node => node.Y) + NodeHeight + 16.0);
		}
	}
