// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Reflection;

namespace Core.UI.ViewModels;

	public sealed class ProjectSettingsGroupViewModel
	{
		public required string Name { get; init; }
		public required string Category { get; init; }
		public ObservableCollection<PropertyInspectorFieldViewModel> Properties { get; } = new();
	}

	public sealed class ProjectSettingsViewModel : NoxUI.ViewModelBase
	{
		private readonly Core.Workspace _Workspace;
		private readonly Core.ProjectSettings _Settings;

		public ObservableCollection<ProjectSettingsGroupViewModel> Groups { get; } = new();
		public NoxUI.ViewModelCommand SaveCommand => field ??= new(Save, CanSave);

		public bool IsDirty
		{
			get => field;
			private set
			{
				if (SetProperty(ref field, value))
				{
					SaveCommand.RaiseCanExecuteChanged();
					RaisePropertyChanged(nameof(StatusText));
				}
			}
		}

		public string StatusText => IsDirty ? "Unsaved changes" : "Saved";

		public ProjectSettingsViewModel()
		{
			_Workspace = Core.StudioManager.Instance.Workspace;
			_Settings = Core.StudioManager.Instance.ProjectSettings;
			BuildGroups();
		}

		public bool ConfirmClose()
		{
			if (IsDirty == false)
			{
				return true;
			}

			System.Windows.MessageBoxResult result = Core.UI.MessageBox.ShowDialog(
				"Project settings have unsaved changes. Save before closing?",
				"Project Settings",
				System.Windows.MessageBoxImage.Question,
				System.Windows.MessageBoxButton.YesNoCancel);

			if (result == System.Windows.MessageBoxResult.Cancel)
			{
				return false;
			}

			if (result == System.Windows.MessageBoxResult.Yes)
			{
				Save();
			}

			return true;
		}

		private void BuildGroups()
		{
			Groups.Clear();
			AddGroup("Project", "Project", _Settings);

			foreach (Core.ProjectSettingRuntime setting in _Settings.RuntimeSettings.OrderBy(static setting => setting.Name, StringComparer.Ordinal))
			{
				AddGroup(setting.Name, "Runtime", setting);
			}

			foreach (Core.ProjectSettingEditor setting in _Settings.EditorSettings.OrderBy(static setting => setting.Name, StringComparer.Ordinal))
			{
				AddGroup(setting.Name, "Editor", setting);
			}
		}

		private void AddGroup(string name, string category, object source)
		{
			ProjectSettingsGroupViewModel group = new()
			{
				Name = name,
				Category = category,
			};

			foreach (PropertyInfo property in source.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance)
				.Where(static property => property.CanRead && property.CanWrite && property.GetIndexParameters().Length == 0 && IsSupportedPropertyType(property.PropertyType))
				.OrderBy(static property => property.Name, StringComparer.Ordinal))
			{
				group.Properties.Add(CreatePropertyField(source, property));
			}

			if (group.Properties.Count > 0)
			{
				Groups.Add(group);
			}
		}

		private void MarkDirty()
		{
			IsDirty = true;
		}

		private bool CanSave()
		{
			return IsDirty;
		}

		private void Save()
		{
			_Settings.Save(_Workspace.ProjectPath);
			IsDirty = false;
		}

		private PropertyInspectorFieldViewModel CreatePropertyField(object source, PropertyInfo property)
		{
			Type propertyType = Nullable.GetUnderlyingType(property.PropertyType) ?? property.PropertyType;
			PropertyInspectorFieldViewModel fieldViewModel;
			if (propertyType == typeof(bool))
			{
				PropertyInspectorBooleanFieldViewModel? booleanField = null;
				booleanField = new PropertyInspectorBooleanFieldViewModel(value => SetSourceValue(source, property, value, booleanField!));
				booleanField.SetInitialValue(property.GetValue(source) is true);
				fieldViewModel = booleanField;
			}
			else if (propertyType.IsEnum)
			{
				PropertyInspectorEnumFieldViewModel? enumField = null;
				enumField = new PropertyInspectorEnumFieldViewModel(Enum.GetNames(propertyType), value => SetConvertedValue(source, property, value, enumField!));
				enumField.SetInitialValue(Convert.ToString(property.GetValue(source), CultureInfo.InvariantCulture) ?? string.Empty);
				fieldViewModel = enumField;
			}
			else
			{
				PropertyInspectorTextFieldViewModel? textField = null;
				textField = new PropertyInspectorTextFieldViewModel(value => SetConvertedValue(source, property, value, textField!));
				textField.SetInitialValue(Convert.ToString(property.GetValue(source), CultureInfo.InvariantCulture) ?? string.Empty);
				textField.IsMultiline = propertyType == typeof(string);
				fieldViewModel = textField;
			}

			fieldViewModel.Name = property.Name;
			fieldViewModel.DisplayName = property.Name;
			fieldViewModel.TypeName = GetTypeName(property.PropertyType);
			return fieldViewModel;
		}

		private bool SetConvertedValue(object source, PropertyInfo property, string value, PropertyInspectorFieldViewModel fieldViewModel)
		{
			try
			{
				Type propertyType = Nullable.GetUnderlyingType(property.PropertyType) ?? property.PropertyType;
				object? convertedValue = propertyType.IsEnum
					? Enum.Parse(propertyType, value)
					: TypeDescriptor.GetConverter(propertyType).ConvertFrom(null, CultureInfo.InvariantCulture, value);
				return SetSourceValue(source, property, convertedValue, fieldViewModel);
			}
			catch (Exception ex) when (ex is ArgumentException or FormatException or NotSupportedException)
			{
				fieldViewModel.SetError(ex.Message);
				return false;
			}
		}

		private bool SetSourceValue(object source, PropertyInfo property, object? value, PropertyInspectorFieldViewModel fieldViewModel)
		{
			object? currentValue = property.GetValue(source);
			if (Equals(currentValue, value))
			{
				fieldViewModel.SetError(string.Empty);
				return false;
			}

			property.SetValue(source, value);
			fieldViewModel.SetError(string.Empty);
			MarkDirty();
			return true;
		}

		private static bool IsSupportedPropertyType(Type type)
		{
			Type propertyType = Nullable.GetUnderlyingType(type) ?? type;
			return propertyType == typeof(string) ||
				propertyType == typeof(bool) ||
				propertyType.IsEnum ||
				propertyType == typeof(byte) ||
				propertyType == typeof(short) ||
				propertyType == typeof(int) ||
				propertyType == typeof(long) ||
				propertyType == typeof(float) ||
				propertyType == typeof(double) ||
				propertyType == typeof(decimal);
		}

		private static string GetTypeName(Type type)
		{
			Type propertyType = Nullable.GetUnderlyingType(type) ?? type;
			return propertyType.IsEnum ? "enum" : propertyType.Name;
		}
	}
