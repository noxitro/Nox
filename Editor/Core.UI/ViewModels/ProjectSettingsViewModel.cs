using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Reflection;

namespace Core.UI.ViewModels
{
	public sealed class ProjectSettingsGroupViewModel
	{
		public required string Name { get; init; }
		public required string Category { get; init; }
		public ObservableCollection<ProjectSettingsPropertyViewModel> Properties { get; } = new();
	}

	public sealed class ProjectSettingsPropertyViewModel : NoxUI.ViewModelBase
	{
		private readonly object _Source;
		private readonly PropertyInfo _Property;
		private readonly Action _MarkDirty;

		public string Name => _Property.Name;
		public string TypeName => GetTypeName(_Property.PropertyType);
		public bool IsBoolean => _Property.PropertyType == typeof(bool);
		public bool IsEnum => _Property.PropertyType.IsEnum;
		public bool IsText => IsBoolean == false && IsEnum == false;
		public string[] EnumValues { get; }

		public string ValueText
		{
			get => Convert.ToString(_Property.GetValue(_Source), CultureInfo.InvariantCulture) ?? string.Empty;
			set
			{
				if (SetValue(value))
				{
					RaisePropertyChanged();
				}
			}
		}

		public bool BoolValue
		{
			get => _Property.GetValue(_Source) is true;
			set
			{
				if (SetSourceValue(value))
				{
					RaisePropertyChanged();
				}
			}
		}

		public string SelectedEnumValue
		{
			get => Convert.ToString(_Property.GetValue(_Source), CultureInfo.InvariantCulture) ?? string.Empty;
			set
			{
				if (SetValue(value))
				{
					RaisePropertyChanged();
				}
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

		public ProjectSettingsPropertyViewModel(object source, PropertyInfo property, Action markDirty)
		{
			_Source = source;
			_Property = property;
			_MarkDirty = markDirty;
			EnumValues = property.PropertyType.IsEnum ? Enum.GetNames(property.PropertyType) : Array.Empty<string>();
		}

		private bool SetValue(string value)
		{
			try
			{
				Type propertyType = Nullable.GetUnderlyingType(_Property.PropertyType) ?? _Property.PropertyType;
				object? convertedValue = propertyType.IsEnum
					? Enum.Parse(propertyType, value)
					: TypeDescriptor.GetConverter(propertyType).ConvertFrom(null, CultureInfo.InvariantCulture, value);
				return SetSourceValue(convertedValue);
			}
			catch (Exception ex) when (ex is ArgumentException or FormatException or NotSupportedException)
			{
				ErrorText = ex.Message;
				return false;
			}
		}

		private bool SetSourceValue(object? value)
		{
			object? currentValue = _Property.GetValue(_Source);
			if (Equals(currentValue, value))
			{
				ErrorText = string.Empty;
				return false;
			}

			_Property.SetValue(_Source, value);
			ErrorText = string.Empty;
			_MarkDirty();
			return true;
		}

		private static string GetTypeName(Type type)
		{
			Type propertyType = Nullable.GetUnderlyingType(type) ?? type;
			return propertyType.IsEnum ? "enum" : propertyType.Name;
		}
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
				group.Properties.Add(new ProjectSettingsPropertyViewModel(source, property, MarkDirty));
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
	}
}
