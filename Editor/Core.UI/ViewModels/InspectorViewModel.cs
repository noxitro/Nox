// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using Core.RuntimeAttributes;
using Nox.Extensions;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Numerics;
using System.Threading;
using System.Windows.Threading;

namespace Core.UI.ViewModels;

	public sealed class ComponentTypeOption
	{
		public required string DisplayName { get; init; }
		public required string FullName { get; init; }
	}

	public sealed class InspectorTreeNodeViewModel : NoxUI.ViewModelBase
	{
		public ObservableCollection<InspectorTreeNodeViewModel> Children { get; } = new();
		public PropertyInspectorFieldViewModel? Field { get; init; }
		public required string DisplayName { get; init; }
		public string TypeName { get; init; } = string.Empty;
		public string Icon { get; init; } = "";
		public bool IsExpanded
		{
			get => field;
			set => SetProperty(ref field, value);
		} = true;
		public bool HasField => Field != null;
		public string ToolTipText => Field?.ToolTipText ?? TypeName;
	}

	public sealed class InspectorViewModel : NoxUI.ViewModelBase, IDisposable
	{
		#region 非公開フィールド
		private readonly Core.SelectionService _Selection;
		private readonly Core.SceneHierarchyManager _SceneHierarchy;
		private readonly InspectorSyncSettings _SyncSettings;
		private readonly DispatcherTimer _AutoSyncTimer;
		private int _AutoSyncPendingCount;
		private int _AutoSyncHasChanges;
		private NoxUI.ViewModelCommand? _AddComponentCommand;
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public ObservableCollection<PropertyInspectorFieldViewModel> Properties { get; } = new();
		public ObservableCollection<InspectorTreeNodeViewModel> TreeNodes { get; } = new();
		public ObservableCollection<ComponentTypeOption> AvailableComponentTypes { get; } = new();

		public string Title
		{
			get => field;
			set => SetProperty(ref field, value);
		} = "Nothing selected";

		public string Description
		{
			get => field;
			set => SetProperty(ref field, value);
		} = "Select a hierarchy object or asset.";

		public string Icon
		{
			get => field;
			set => SetProperty(ref field, value);
		} = "";

		public string AutoSyncStatus
		{
			get => field;
			set => SetProperty(ref field, value);
		} = "Auto-Sync: Off";

		public bool CanAddComponents
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					_AddComponentCommand?.RaiseCanExecuteChanged();
				}
			}
		}

		public ComponentTypeOption? SelectedComponentType
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value) && value != null)
				{
					ComponentTypeText = value.FullName;
				}
			}
		}

		public string ComponentTypeText
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					_AddComponentCommand?.RaiseCanExecuteChanged();
				}
			}
		} = string.Empty;

		public NoxUI.ViewModelCommand AddComponentCommand => _AddComponentCommand ??= new(AddComponent, CanAddComponent);
		#endregion

		public InspectorViewModel()
		{
			_Selection = Core.StudioManager.Instance.Workspace.Selection;
			_SceneHierarchy = Core.StudioManager.Instance.Workspace.SceneHierarchy;
			_SyncSettings = InspectorSyncSettings.Instance;
			_Selection.Changed += OnSelectionChanged;
			_SyncSettings.Changed += OnSyncSettingsChanged;
			_SyncSettings.ManualSyncRequested += OnManualSyncRequested;
			_AutoSyncTimer = new DispatcherTimer
			{
				Interval = TimeSpan.FromMilliseconds(_SyncSettings.SelectedOption.Milliseconds),
			};
			_AutoSyncTimer.Tick += OnAutoSyncTick;
			PopulateComponentTypes();
			Refresh(_Selection.Current);
		}

		#region 非公開メソッド
		private void OnSelectionChanged(object? sender, EventArgs e)
		{
			Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (_Disposed || NoxUI.DispatcherHelper.IsShuttingDown(dispatcher))
			{
				return;
			}

			if (dispatcher!.CheckAccess() == false)
			{
				NoxUI.DispatcherHelper.TryBeginInvoke(dispatcher, () => Refresh(_Selection.Current));
				return;
			}

			Refresh(_Selection.Current);
		}

		private void Refresh(Core.SelectionInfo selection)
		{
			Interlocked.Exchange(ref _AutoSyncHasChanges, 0);
			if (selection.Value is not Core.SceneHierarchyNode)
			{
				_AutoSyncPendingCount = 0;
			}

			Properties.Clear();
			TreeNodes.Clear();
			Title = selection.DisplayName;
			Description = selection.Description;
			CanAddComponents = false;

			switch (selection.Value)
			{
				case Core.SceneHierarchyNode node:
					Icon = GetHierarchyIcon(node.Kind);
					CanAddComponents = node.Kind == Core.SceneHierarchyNodeKind.EntityNode;
					AddComponentTree(node);
					SelectDefaultComponentType(node);
					UpdateAutoSyncTimer(node.RemoteInstanceId);
					break;
				case Core.RuntimeObject runtimeObject:
					Icon = "";
					TreeNodes.Add(CreateRuntimeObjectTreeNode(runtimeObject));
					UpdateAutoSyncTimer(runtimeObject.RemoteInstanceId);
					break;
				case Core.ProjectAsset asset:
					Icon = Core.AssetTypeUtility.GetIconGlyph(asset.Extension);
					AutoSyncStatus = "Auto-Sync: Off";
					_AutoSyncTimer.Stop();
					break;
				default:
					Icon = "";
					AutoSyncStatus = "Auto-Sync: Off";
					_AutoSyncTimer.Stop();
					break;
			}
		}

		private void AddComponentTree(Core.SceneHierarchyNode node)
		{
			foreach (Core.RuntimeObject component in node.Components)
			{
				TreeNodes.Add(CreateRuntimeObjectTreeNode(component));
			}
		}

		private InspectorTreeNodeViewModel CreateRuntimeObjectTreeNode(Core.RuntimeObject runtimeObject)
		{
			InspectorTreeNodeViewModel node = new()
			{
				DisplayName = GetRuntimeObjectDisplayName(runtimeObject),
				TypeName = runtimeObject.RuntimeRecordDecl.FullName,
				Icon = "",
			};
			AddRuntimeVariableNodes(node, runtimeObject);
			AddRuntimeActionNodes(node, runtimeObject);
			return node;
		}

		private void AddRuntimeVariableNodes(InspectorTreeNodeViewModel parent, Core.RuntimeObject runtimeObject)
		{
			ReadOnlySpan<Core.RuntimePropertyDecl> propertyList = runtimeObject.RuntimeRecordDecl.PropertyList;
			ReadOnlySpan<object> valueList = runtimeObject.PropertyValueList;
			for (int i = 0; i < propertyList.Length; ++i)
			{
				Core.RuntimePropertyDecl propertyDecl = propertyList[i];
				object? value = i < valueList.Length ? valueList[i] : null;
				bool isReadOnly = HasAttribute<ReadOnlyAttribute>(propertyDecl.AttributeList) ||
					(propertyDecl.VariableDecl == null && propertyDecl.SetterFunctionDecl == null);
				parent.Children.Add(CreateVariableNode(
					propertyDecl.Name,
					propertyDecl.TypeInfo,
					value,
					isReadOnly ? null : updatedValue =>
					{
						runtimeObject.SetPropertyValue(propertyDecl.Name, updatedValue);
						SyncRuntimeObject(runtimeObject);
					},
					propertyDecl.AttributeList,
					isReadOnly,
					GetRuntimeObjectDisplayName(runtimeObject)));
			}
		}

		private void AddRuntimeActionNodes(InspectorTreeNodeViewModel parent, Core.RuntimeObject runtimeObject)
		{
			foreach (Core.RuntimeFunctionDecl functionDecl in runtimeObject.RuntimeRecordDecl.FunctionList)
			{
				if (IsAction(functionDecl.AttributeList) == false || functionDecl.GetArgumentList().Length != 0)
				{
					continue;
				}

				PropertyInspectorActionFieldViewModel actionField = new(() => InvokeRuntimeAction(runtimeObject, functionDecl))
				{
					Name = functionDecl.Name,
					DisplayName = GetDisplayName(functionDecl.Name, functionDecl.AttributeList),
					Description = GetDescription(functionDecl.AttributeList),
					Category = GetCategory(functionDecl.AttributeList, GetRuntimeObjectDisplayName(runtimeObject)),
				};
				parent.Children.Add(CreateFieldNode(actionField, functionDecl.TypeInfo.FullName, ""));
			}
		}

		private InspectorTreeNodeViewModel CreateVariableNode(
			string name,
			Core.RuntimeTypeInfo typeInfo,
			object? value,
			Action<object?>? commit,
			System.Collections.Generic.IEnumerable<Attribute> attributes,
			bool isReadOnly,
			string categoryFallback)
		{
			PropertyInspectorFieldViewModel? field = CreateField(name, typeInfo, value, commit, attributes, isReadOnly, categoryFallback);
			if (field != null)
			{
				return CreateFieldNode(field, typeInfo.FullName, "");
			}

			InspectorTreeNodeViewModel container = new()
			{
				DisplayName = GetDisplayName(name, attributes),
				TypeName = typeInfo.FullName,
				Icon = "",
			};
			if (typeInfo.Decl is Core.RuntimeRecordDecl recordDecl)
			{
				AddRuntimeRecordDeclNodes(container, recordDecl, categoryFallback);
			}
			return container;
		}

		private void AddRuntimeRecordDeclNodes(InspectorTreeNodeViewModel parent, Core.RuntimeRecordDecl recordDecl, string categoryFallback)
		{
			foreach (Core.RuntimePropertyDecl propertyDecl in recordDecl.PropertyList)
			{
				parent.Children.Add(CreateVariableNode(
					propertyDecl.Name,
					propertyDecl.TypeInfo,
					null,
					null,
					propertyDecl.AttributeList,
					isReadOnly: true,
					categoryFallback));
			}
		}

		private static InspectorTreeNodeViewModel CreateFieldNode(PropertyInspectorFieldViewModel field, string typeName, string icon)
		{
			return new InspectorTreeNodeViewModel
			{
				DisplayName = field.DisplayName,
				TypeName = string.IsNullOrWhiteSpace(field.TypeName) ? typeName : field.TypeName,
				Icon = icon,
				Field = field,
			};
		}

		private void AddField(
			string name,
			Core.RuntimeTypeInfo typeInfo,
			object? value,
			Action<object?>? commit,
			System.Collections.Generic.IEnumerable<Attribute> attributes,
			bool isReadOnly,
			string categoryFallback = "")
		{
			PropertyInspectorFieldViewModel? field = CreateField(name, typeInfo, value, commit, attributes, isReadOnly, categoryFallback);
			if (field != null)
			{
				Properties.Add(field);
			}
		}

		private void AddField(
			string name,
			Type type,
			object? value,
			Action<object?>? commit,
			System.Collections.Generic.IEnumerable<Attribute> attributes,
			bool isReadOnly,
			string categoryFallback = "")
		{
			Properties.Add(CreateField(name, type, value, commit, attributes, isReadOnly, categoryFallback));
		}

		private PropertyInspectorFieldViewModel? CreateField(
			string name,
			Core.RuntimeTypeInfo typeInfo,
			object? value,
			Action<object?>? commit,
			System.Collections.Generic.IEnumerable<Attribute> attributes,
			bool isReadOnly,
			string categoryFallback = "")
		{
			Type? clrType = GetKnownClrType(typeInfo);
			if (clrType != null)
			{
				return CreateField(name, clrType, value, commit, attributes, isReadOnly, categoryFallback);
			}

			if (IsBoolean(typeInfo))
			{
				return CreateField(name, typeof(bool), value, commit, attributes, isReadOnly, categoryFallback);
			}

			if (typeInfo.Decl is Core.RuntimeRecordDecl)
			{
				return null;
			}

			return CreateReadOnlyField(name, value?.ToString() ?? $"<{typeInfo.FullName}>", attributes, categoryFallback);
		}

		private PropertyInspectorFieldViewModel CreateField(
			string name,
			Type type,
			object? value,
			Action<object?>? commit,
			System.Collections.Generic.IEnumerable<Attribute> attributes,
			bool isReadOnly,
			string categoryFallback = "")
		{
			PropertyInspectorFieldViewModel fieldViewModel;
			if (type == typeof(bool))
			{
				PropertyInspectorBooleanFieldViewModel booleanField = new(commit == null ? null : updatedValue => commit(updatedValue));
				booleanField.SetInitialValue(value is bool boolValue && boolValue);
				fieldViewModel = booleanField;
			}
			else if (type == typeof(string))
			{
				PropertyInspectorTextFieldViewModel textField = new(commit == null ? null : updatedValue => commit(updatedValue));
				textField.SetInitialValue(value?.ToString() ?? string.Empty);
				textField.IsMultiline = (value?.ToString()?.Length ?? 0) > 48;
				fieldViewModel = textField;
			}
			else if (TryGetVector3(value, type, out double x, out double y, out double z))
			{
				PropertyInspectorVector3FieldViewModel vectorField = new(commit == null ? null : (updatedX, updatedY, updatedZ) => commit(CreateVectorValue(type, updatedX, updatedY, updatedZ)));
				vectorField.SetInitialValue(x, y, z);
				fieldViewModel = vectorField;
			}
			else if (IsNumericType(type))
			{
				PropertyInspectorTextFieldViewModel textField = new(commit == null ? null : updatedValue =>
				{
					if (TryChangeType(updatedValue, type, out object? convertedValue))
					{
						commit(convertedValue);
					}
				});
				textField.SetInitialValue(FormatValue(value, type));
				fieldViewModel = textField;
			}
			else
			{
				return CreateReadOnlyField(name, FormatValue(value, type), attributes, categoryFallback);
			}

			ApplyFieldMetadata(fieldViewModel, name, attributes, categoryFallback);
			fieldViewModel.IsReadOnly = isReadOnly;
			return fieldViewModel;
		}

		private void AddReadOnlyField(string name, string value, System.Collections.Generic.IEnumerable<Attribute>? attributes = null, string categoryFallback = "")
		{
			Properties.Add(CreateReadOnlyField(name, value, attributes, categoryFallback));
		}

		private PropertyInspectorReadOnlyFieldViewModel CreateReadOnlyField(string name, string value, System.Collections.Generic.IEnumerable<Attribute>? attributes = null, string categoryFallback = "")
		{
			PropertyInspectorReadOnlyFieldViewModel fieldViewModel = new()
			{
				Value = value,
				IsReadOnly = true,
			};
			ApplyFieldMetadata(fieldViewModel, name, attributes ?? [], categoryFallback);
			return fieldViewModel;
		}

		private void InvokeRuntimeAction(Core.RuntimeObject runtimeObject, Core.RuntimeFunctionDecl functionDecl)
		{
			Core.Net.RuntimeRemoteClient remoteClient =
				runtimeObject.RemoteClient ?? Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession().RemoteClient;
			if (remoteClient.IsConnected == false || runtimeObject.RemoteInstanceId == 0)
			{
				return;
			}

			remoteClient.SendQuery(new Core.RuntimeRemote.InvokeRuntimeActionQuery
			{
				RemoteInstanceId = runtimeObject.RemoteInstanceId,
				FunctionFullName = functionDecl.FullName,
			}, null);
		}

		private void OnAutoSyncTick(object? sender, EventArgs e)
		{
			RunAutoSync();
		}

		private void RunAutoSync()
		{
			if (Volatile.Read(ref _AutoSyncPendingCount) > 0)
			{
				return;
			}

			switch (_Selection.Current.Value)
			{
				case Core.SceneHierarchyNode node:
					AutoSyncRemoteObject(node.RemoteObject, removeMissingNode: true);
					foreach (Core.RuntimeObject component in node.Components)
					{
						AutoSyncRemoteObject(component, removeMissingNode: false);
					}
					break;
				case Core.RuntimeObject selectedObject:
					AutoSyncRemoteObject(selectedObject, removeMissingNode: false);
					break;
			}
		}

		private void AutoSyncRemoteObject(Core.RuntimeObject? remoteObject, bool removeMissingNode)
		{
			if (remoteObject == null || remoteObject.RemoteInstanceId == 0)
			{
				return;
			}

			Core.Net.RuntimeRemoteClient remoteClient = remoteObject.RemoteClient ??
				Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession().RemoteClient;
			if (remoteClient.IsConnected == false)
			{
				return;
			}

			Interlocked.Increment(ref _AutoSyncPendingCount);
			try
			{
				remoteClient.SendQuery(new Core.RuntimeRemote.AutoSyncQuery
				{
					RemoteInstanceId = remoteObject.RemoteInstanceId,
				}, response =>
				{
					Core.RuntimeRemote.AutoSyncResponse autoSyncResponse = Nox.Util.Cast<Core.RuntimeRemote.AutoSyncResponse>(response);
					if (autoSyncResponse.Exists == false)
					{
						if (removeMissingNode)
						{
							Dispatcher? removeDispatcher = System.Windows.Application.Current?.Dispatcher;
							void RemoveMissingNode() => Core.StudioManager.Instance.Workspace.SceneHierarchy.RemoveByRemoteInstanceId(autoSyncResponse.RemoteInstanceId, syncRuntime: false);
							if (_Disposed || NoxUI.DispatcherHelper.IsShuttingDown(removeDispatcher))
							{
								CompleteAutoSyncResponse();
								return;
							}

							if (removeDispatcher!.CheckAccess() == false)
							{
								NoxUI.DispatcherHelper.TryBeginInvoke(removeDispatcher, RemoveMissingNode);
							}
							else
							{
								RemoveMissingNode();
							}
						}
						CompleteAutoSyncResponse();
						return;
					}

					if (Core.RuntimeRemote.Util.SetPropertiesFromBytes(autoSyncResponse.PropertyByteBuffer, remoteObject))
					{
						Interlocked.Exchange(ref _AutoSyncHasChanges, 1);
					}
					CompleteAutoSyncResponse();
				});
			}
			catch
			{
				CompleteAutoSyncResponse();
				throw;
			}
		}

		private void CompleteAutoSyncResponse()
		{
			if (_Disposed)
			{
				return;
			}

			int pendingCount = Interlocked.Decrement(ref _AutoSyncPendingCount);
			if (pendingCount < 0)
			{
				Interlocked.Exchange(ref _AutoSyncPendingCount, 0);
				pendingCount = 0;
			}
			if (pendingCount > 0)
			{
				return;
			}
			if (Interlocked.Exchange(ref _AutoSyncHasChanges, 0) == 0)
			{
				return;
			}

			Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (NoxUI.DispatcherHelper.IsShuttingDown(dispatcher))
			{
				return;
			}

			if (dispatcher!.CheckAccess() == false)
			{
				NoxUI.DispatcherHelper.TryBeginInvoke(dispatcher, () => Refresh(_Selection.Current));
				return;
			}

			Refresh(_Selection.Current);
		}

		private void UpdateAutoSyncTimer(long remoteInstanceId)
		{
			if (remoteInstanceId == 0)
			{
				AutoSyncStatus = "Auto-Sync: Waiting for runtime instance";
				_AutoSyncTimer.Stop();
				return;
			}

			InspectorSyncIntervalOption option = _SyncSettings.SelectedOption;
			if (option.IsManual)
			{
				AutoSyncStatus = "Auto-Sync: Manual";
				_AutoSyncTimer.Stop();
				return;
			}

			_AutoSyncTimer.Interval = TimeSpan.FromMilliseconds(option.Milliseconds);
			AutoSyncStatus = $"Auto-Sync: {option.Milliseconds.ToString(CultureInfo.InvariantCulture)}ms";
			_AutoSyncTimer.Start();
		}

		public override void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			_Selection.Changed -= OnSelectionChanged;
			_SyncSettings.Changed -= OnSyncSettingsChanged;
			_SyncSettings.ManualSyncRequested -= OnManualSyncRequested;
			_AutoSyncTimer.Stop();
			_AutoSyncTimer.Tick -= OnAutoSyncTick;
			_Disposed = true;
		}

		private void OnSyncSettingsChanged(object? sender, EventArgs e)
		{
			Refresh(_Selection.Current);
		}

		private void OnManualSyncRequested(object? sender, EventArgs e)
		{
			RunAutoSync();
		}

		private bool CanAddComponent()
		{
			return CanAddComponents && string.IsNullOrWhiteSpace(ComponentTypeText) == false;
		}

		private void AddComponent()
		{
			if (_Selection.Current.Value is not Core.SceneHierarchyNode node)
			{
				return;
			}

			Core.RuntimeObject? component = _SceneHierarchy.AddComponent(node, ComponentTypeText.Trim());
			if (component != null)
			{
				Refresh(_Selection.Current);
			}
		}

		private void PopulateComponentTypes()
		{
			AvailableComponentTypes.Clear();
			foreach (Core.RuntimeRecordDecl recordDecl in Core.StudioManager.Instance.GetEngineSystem<Core.Runtime>().TypeDB.GetRecordDeclList()
				.Where(IsComponentCandidate)
				.OrderBy(static decl => decl.FullName, StringComparer.Ordinal))
			{
				AvailableComponentTypes.Add(new ComponentTypeOption
				{
					DisplayName = GetRuntimeObjectDisplayName(recordDecl),
					FullName = recordDecl.FullName,
				});
			}
		}

		private void SelectDefaultComponentType(Core.SceneHierarchyNode node)
		{
			if (SelectedComponentType != null || string.IsNullOrWhiteSpace(ComponentTypeText) == false)
			{
				return;
			}

			foreach (ComponentTypeOption option in AvailableComponentTypes)
			{
				if (node.HasComponent(option.FullName) == false)
				{
					SelectedComponentType = option;
					return;
				}
			}
		}

		private static bool IsComponentCandidate(Core.RuntimeRecordDecl recordDecl)
		{
			if (recordDecl.FullName == "nox::Component" || recordDecl.FullName == "nox::EntityNode")
			{
				return false;
			}

			return recordDecl.FullName == "nox::Transform" ||
				recordDecl.FullName == "nox::Behavior" ||
				recordDecl.Name.EndsWith("Behavior", StringComparison.Ordinal) ||
				recordDecl.Name.EndsWith("Behaviour", StringComparison.Ordinal) ||
				recordDecl.Name.EndsWith("Component", StringComparison.Ordinal);
		}

		private static void SyncRuntimeObject(Core.RuntimeObject runtimeObject)
		{
			if (runtimeObject.RemoteInstanceId != 0)
			{
				runtimeObject.Sync(Core.Net.SyncMode.OneWay);
			}
		}

		private static void ApplyFieldMetadata(PropertyInspectorFieldViewModel fieldViewModel, string fallbackName, System.Collections.Generic.IEnumerable<Attribute> attributes, string categoryFallback = "")
		{
			fieldViewModel.Name = fallbackName;
			fieldViewModel.DisplayName = GetDisplayName(fallbackName, attributes);
			fieldViewModel.Description = GetDescription(attributes);
			fieldViewModel.Category = GetCategory(attributes, categoryFallback);
		}

		private static string GetDisplayName(string fallbackName, System.Collections.Generic.IEnumerable<Attribute> attributes)
		{
			DisplayNameAttribute? displayNameAttribute = attributes.OfType<DisplayNameAttribute>().FirstOrDefault();
			return string.IsNullOrWhiteSpace(displayNameAttribute?.DisplayName) ? fallbackName : displayNameAttribute.DisplayName;
		}

		private static string GetDescription(System.Collections.Generic.IEnumerable<Attribute> attributes)
		{
			DescriptionAttribute? descriptionAttribute = attributes.OfType<DescriptionAttribute>().FirstOrDefault();
			return descriptionAttribute?.Description ?? string.Empty;
		}

		private static string GetCategory(System.Collections.Generic.IEnumerable<Attribute> attributes, string fallback = "")
		{
			CategoryAttribute? categoryAttribute = attributes.OfType<CategoryAttribute>().FirstOrDefault();
			return string.IsNullOrWhiteSpace(categoryAttribute?.Category) ? fallback : categoryAttribute.Category;
		}

		private static string GetRuntimeObjectDisplayName(Core.RuntimeObject runtimeObject)
		{
			return GetRuntimeObjectDisplayName(runtimeObject.RuntimeRecordDecl);
		}

		private static string GetRuntimeObjectDisplayName(Core.RuntimeRecordDecl runtimeRecordDecl)
		{
			string name = runtimeRecordDecl.Name;
			if (string.IsNullOrWhiteSpace(name))
			{
				name = runtimeRecordDecl.FullName;
			}

			int scopeIndex = name.LastIndexOf("::", StringComparison.Ordinal);
			return scopeIndex >= 0 ? name[(scopeIndex + 2)..] : name;
		}

		private static bool IsHidden(System.Collections.Generic.IEnumerable<Attribute> attributes)
		{
			return attributes.OfType<Core.RuntimeAttributes.HideAttribute>().Any();
		}

		private static bool IsAction(System.Collections.Generic.IEnumerable<Attribute> attributes)
		{
			return attributes.OfType<Core.RuntimeAttributes.ActionAttribute>().Any() ||
				attributes.Any(attribute => attribute.GetType().Name.Equals("ActionAttribute", StringComparison.Ordinal));
		}

		private static bool HasAttribute<T>(System.Collections.Generic.IEnumerable<Attribute> attributes) where T : Attribute
		{
			return attributes.OfType<T>().Any();
		}

		private static Type? GetKnownClrType(Core.RuntimeTypeInfo typeInfo)
		{
			return typeInfo.FullName switch
			{
				"nox::Vec3" => typeof(Vector3),
				"nox::detail::Vector3D<float>" => typeof(Vector3),
				"nox::Vec3d" => typeof(Nox.Math.Double3),
				"nox::detail::Vector3D<double>" => typeof(Nox.Math.Double3),
				"nox::Position" => typeof(Nox.Position),
				"nox::Quat" => typeof(Nox.Math.Float4),
				"nox::detail::Quaternion<float>" => typeof(Nox.Math.Float4),
				_ => null,
			};
		}

		private static bool IsBoolean(Core.RuntimeTypeInfo typeInfo)
		{
			return typeInfo.TypeKind == Core.RuntimeTypeKind.Bool;
		}

		private static bool TryGetVector3(object? value, Type type, out double x, out double y, out double z)
		{
			x = 0.0;
			y = 0.0;
			z = 0.0;
			if (type == typeof(Vector3))
			{
				Vector3 vector = value is Vector3 vectorValue ? vectorValue : default;
				x = vector.X;
				y = vector.Y;
				z = vector.Z;
				return true;
			}
			if (type == typeof(Nox.Position))
			{
				Nox.Position position = value is Nox.Position positionValue ? positionValue : default;
				x = position.x;
				y = position.y;
				z = position.z;
				return true;
			}
			if (type == typeof(Nox.Math.Float3))
			{
				Nox.Math.Float3 vector = value is Nox.Math.Float3 vectorValue ? vectorValue : default;
				x = vector.x;
				y = vector.y;
				z = vector.z;
				return true;
			}
			if (type == typeof(Nox.Math.Double3))
			{
				Nox.Math.Double3 vector = value is Nox.Math.Double3 vectorValue ? vectorValue : default;
				x = vector.x;
				y = vector.y;
				z = vector.z;
				return true;
			}
			if (type == typeof(Nox.Math.Float4))
			{
				Nox.Math.Float4 quaternion = value is Nox.Math.Float4 quaternionValue ? quaternionValue : new Nox.Math.Float4 { w = 1.0f };
				Vector3 eulerDegrees = QuaternionToEulerDegrees(quaternion);
				x = eulerDegrees.X;
				y = eulerDegrees.Y;
				z = eulerDegrees.Z;
				return true;
			}

			return false;
		}

		private static object CreateVectorValue(Type type, double x, double y, double z)
		{
			if (type == typeof(Vector3))
			{
				return new Vector3((float)x, (float)y, (float)z);
			}
			if (type == typeof(Nox.Position))
			{
				return new Nox.Position { x = x, y = y, z = z };
			}
			if (type == typeof(Nox.Math.Float3))
			{
				return new Nox.Math.Float3 { x = (float)x, y = (float)y, z = (float)z };
			}
			if (type == typeof(Nox.Math.Double3))
			{
				return new Nox.Math.Double3 { x = x, y = y, z = z };
			}
			if (type == typeof(Nox.Math.Float4))
			{
				return EulerDegreesToQuaternion(x, y, z);
			}

			return default(Vector3);
		}

		private static Nox.Math.Float4 EulerDegreesToQuaternion(double x, double y, double z)
		{
			double halfX = DegreesToRadians(x) * 0.5;
			double halfY = DegreesToRadians(y) * 0.5;
			double halfZ = DegreesToRadians(z) * 0.5;
			double cx = Math.Cos(halfX);
			double sx = Math.Sin(halfX);
			double cy = Math.Cos(halfY);
			double sy = Math.Sin(halfY);
			double cz = Math.Cos(halfZ);
			double sz = Math.Sin(halfZ);

			return new Nox.Math.Float4
			{
				x = (float)(sx * cy * cz + cx * sy * sz),
				y = (float)(cx * sy * cz - sx * cy * sz),
				z = (float)(cx * cy * sz + sx * sy * cz),
				w = (float)(cx * cy * cz - sx * sy * sz),
			};
		}

		private static Vector3 QuaternionToEulerDegrees(Nox.Math.Float4 quaternion)
		{
			double sinrCosp = 2.0 * (quaternion.w * quaternion.x + quaternion.y * quaternion.z);
			double cosrCosp = 1.0 - 2.0 * (quaternion.x * quaternion.x + quaternion.y * quaternion.y);
			double roll = Math.Atan2(sinrCosp, cosrCosp);

			double sinp = 2.0 * (quaternion.w * quaternion.y - quaternion.z * quaternion.x);
			double pitch = Math.Abs(sinp) >= 1.0
				? Math.CopySign(Math.PI / 2.0, sinp)
				: Math.Asin(sinp);

			double sinyCosp = 2.0 * (quaternion.w * quaternion.z + quaternion.x * quaternion.y);
			double cosyCosp = 1.0 - 2.0 * (quaternion.y * quaternion.y + quaternion.z * quaternion.z);
			double yaw = Math.Atan2(sinyCosp, cosyCosp);

			return new Vector3(
				(float)RadiansToDegrees(roll),
				(float)RadiansToDegrees(pitch),
				(float)RadiansToDegrees(yaw));
		}

		private static double DegreesToRadians(double degrees)
		{
			return degrees * Math.PI / 180.0;
		}

		private static double RadiansToDegrees(double radians)
		{
			return radians * 180.0 / Math.PI;
		}

		private static string FormatValue(object? value, Type type)
		{
			if (value == null)
			{
				return string.Empty;
			}

			if (TryGetVector3(value, type, out double x, out double y, out double z))
			{
				return $"{x.ToString(CultureInfo.InvariantCulture)}, {y.ToString(CultureInfo.InvariantCulture)}, {z.ToString(CultureInfo.InvariantCulture)}";
			}

			return Convert.ToString(value, CultureInfo.InvariantCulture) ?? string.Empty;
		}

		private static bool IsNumericType(Type type)
		{
			type = Nullable.GetUnderlyingType(type) ?? type;
			return type == typeof(byte) ||
				type == typeof(sbyte) ||
				type == typeof(short) ||
				type == typeof(ushort) ||
				type == typeof(int) ||
				type == typeof(uint) ||
				type == typeof(long) ||
				type == typeof(ulong) ||
				type == typeof(float) ||
				type == typeof(double) ||
				type == typeof(decimal);
		}

		private static bool TryChangeType(string value, Type targetType, out object? convertedValue)
		{
			try
			{
				convertedValue = Convert.ChangeType(value, Nullable.GetUnderlyingType(targetType) ?? targetType, CultureInfo.InvariantCulture);
				return true;
			}
			catch (FormatException)
			{
				convertedValue = null;
				return false;
			}
			catch (OverflowException)
			{
				convertedValue = null;
				return false;
			}
		}

		private static string GetHierarchyIcon(Core.SceneHierarchyNodeKind kind)
		{
			return kind switch
			{
				Core.SceneHierarchyNodeKind.SceneNode => "",
				Core.SceneHierarchyNodeKind.GroupNode => "",
				_ => "",
			};
		}

		#endregion
	}
