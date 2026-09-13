// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Serialization;
using System.Text.Json;

namespace Core;

	public enum ProjectSettingTabKind : byte
	{
		/// <summary>
		/// runtime側設定
		/// </summary>
		Runtime,

		/// <summary>
		/// editor側設定
		/// </summary>
		Editor,
	}

	public enum PlatformType : byte
	{
		X64,
	}

	public enum ConfigurationType : byte
	{
		Debug,
		Release,
		Master
	}

	/// <summary>
	/// プロジェクト設定
	/// </summary>
	public class ProjectSettings
	{
		#region 公開定数
		public const string ProjectFileExtension = ".noxproj.json";
		#endregion

		#region 非公開フィールド
		private readonly Dictionary<System.Type, ProjectSettingRuntime> _RuntimeSettings = new();
		private readonly Dictionary<System.Type, ProjectSettingEditor> _EditorSettings = new();
		#endregion

		#region 公開プロパティ
		public string AssetFolderName { get; set; } = Workspace.DefaultAssetFolderName;
		public string RuntimeRootRelativePath { get; set; } = "runtime";
		public IReadOnlyCollection<ProjectSettingRuntime> RuntimeSettings => _RuntimeSettings.Values;
		public IReadOnlyCollection<ProjectSettingEditor> EditorSettings => _EditorSettings.Values;
		#endregion

		#region 公開メソッド
		public static ProjectSettings LoadOrCreate(string projectPath)
		{
			Nox.Util.Assert(string.IsNullOrWhiteSpace(projectPath) == false, "ProjectPath is empty.");

			string fullProjectPath = Path.GetFullPath(projectPath);
			Directory.CreateDirectory(fullProjectPath);

			string settingsPath = GetSettingsPath(fullProjectPath);
			ProjectSettings settings = CreateDefault(fullProjectPath);
			if (File.Exists(settingsPath))
			{
				settings.LoadFromJson(settingsPath);
				settings.Normalize(fullProjectPath);
				settings.Save(fullProjectPath);
				return settings;
			}

			settings.Save(fullProjectPath);
			return settings;
		}

		public void Save(string projectPath)
		{
			string fullProjectPath = Path.GetFullPath(projectPath);
			Directory.CreateDirectory(fullProjectPath);
			using FileStream stream = File.Create(GetSettingsPath(fullProjectPath));
			using Utf8JsonWriter writer = new(stream, new JsonWriterOptions { Indented = true });
			WriteJson(writer);
		}

		public T GetRuntimeSetting<T>() where T : ProjectSettingRuntime
		{
			if (_RuntimeSettings.TryGetValue(typeof(T), out var setting) == false)
			{
				throw new InvalidOperationException($"Runtime setting of type {typeof(T).FullName} not found.");
        }
			return (T)setting;
    }

		public T GetEditorSetting<T>() where T : ProjectSettingEditor
		{
			if (_EditorSettings.TryGetValue(typeof(T), out var setting) == false)
			{
				throw new InvalidOperationException($"Editor setting of type {typeof(T).FullName} not found.");
			}
			return (T)setting;
    }
    #endregion

		#region 非公開メソッド
		private static JsonSerializerOptions JsonOptions => new()
		{
			WriteIndented = true,
		};

		private static string GetSettingsPath(string projectPath)
		{
			return Path.Combine(projectPath, GetProjectFileName(projectPath));
		}

		private static string GetProjectFileName(string projectPath)
		{
			string projectName = new DirectoryInfo(projectPath).Name;
			Nox.Util.Assert(string.IsNullOrWhiteSpace(projectName) == false, "Project name is empty.");
			return $"{projectName}{ProjectFileExtension}";
		}

		private static ProjectSettings CreateDefault(string projectPath)
		{
			ProjectSettings settings = new();
			settings.AssetFolderName = ResolveDefaultAssetFolderName(projectPath);
			settings.RuntimeRootRelativePath = ResolveDefaultRuntimeRootRelativePath(projectPath);
			settings.InitializeSettings();
			return settings;
		}

		private void InitializeSettings()
		{
			_RuntimeSettings.Clear();
			_EditorSettings.Clear();

			foreach (Type type in EnumerateSettingTypes<ProjectSettingRuntime>())
			{
				_RuntimeSettings.Add(type, (ProjectSettingRuntime)Activator.CreateInstance(type, nonPublic: true)!);
			}

			foreach (Type type in EnumerateSettingTypes<ProjectSettingEditor>())
			{
				_EditorSettings.Add(type, (ProjectSettingEditor)Activator.CreateInstance(type, nonPublic: true)!);
			}
		}

		private void LoadFromJson(string settingsPath)
		{
			using JsonDocument document = JsonDocument.Parse(File.ReadAllText(settingsPath));
			JsonElement root = document.RootElement;

			if (root.TryGetProperty(nameof(AssetFolderName), out JsonElement assetFolderNameElement) &&
				assetFolderNameElement.ValueKind == JsonValueKind.String)
			{
				AssetFolderName = assetFolderNameElement.GetString() ?? AssetFolderName;
			}

			if (root.TryGetProperty(nameof(RuntimeRootRelativePath), out JsonElement runtimeRootElement) &&
				runtimeRootElement.ValueKind == JsonValueKind.String)
			{
				RuntimeRootRelativePath = runtimeRootElement.GetString() ?? RuntimeRootRelativePath;
			}

			LoadSettingValues(root, nameof(RuntimeSettings), _RuntimeSettings);
			LoadSettingValues(root, nameof(EditorSettings), _EditorSettings);
		}

		private void WriteJson(Utf8JsonWriter writer)
		{
			writer.WriteStartObject();
			writer.WriteString(nameof(AssetFolderName), AssetFolderName);
			writer.WriteString(nameof(RuntimeRootRelativePath), RuntimeRootRelativePath);
			WriteSettingValues(writer, nameof(RuntimeSettings), _RuntimeSettings);
			WriteSettingValues(writer, nameof(EditorSettings), _EditorSettings);
			writer.WriteEndObject();
		}

		private static void LoadSettingValues<TSetting>(JsonElement root, string propertyName, Dictionary<Type, TSetting> settings)
			where TSetting : ProjectSettingBase
		{
			if (root.TryGetProperty(propertyName, out JsonElement settingsElement) == false ||
				settingsElement.ValueKind != JsonValueKind.Object)
			{
				return;
			}

			foreach (Type settingType in settings.Keys.ToArray())
			{
				string key = GetSettingKey(settingType);
				if (settingsElement.TryGetProperty(key, out JsonElement settingElement) == false)
				{
					continue;
				}

				object? deserialized = JsonSerializer.Deserialize(settingElement.GetRawText(), settingType, JsonOptions);
				if (deserialized is TSetting typedSetting)
				{
					settings[settingType] = typedSetting;
				}
			}
		}

		private static void WriteSettingValues<TSetting>(Utf8JsonWriter writer, string propertyName, Dictionary<Type, TSetting> settings)
			where TSetting : ProjectSettingBase
		{
			writer.WritePropertyName(propertyName);
			writer.WriteStartObject();
			foreach ((Type type, TSetting setting) in settings.OrderBy(static pair => GetSettingKey(pair.Key), StringComparer.Ordinal))
			{
				writer.WritePropertyName(GetSettingKey(type));
				JsonSerializer.Serialize(writer, setting, type, JsonOptions);
			}
			writer.WriteEndObject();
		}

		private static IEnumerable<Type> EnumerateSettingTypes<TSetting>() where TSetting : ProjectSettingBase
		{
			Type baseType = typeof(TSetting);
			List<Type> settingTypes = new();
			foreach (Type type in Core.TypeDB.AllTypeList)
			{
				if (type == baseType ||
					baseType.IsAssignableFrom(type) == false ||
					type.IsAbstract ||
					type.GetConstructor(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, binder: null, Type.EmptyTypes, modifiers: null) == null)
				{
					continue;
				}

				settingTypes.Add(type);
			}

			settingTypes.Sort(static (lhs, rhs) => string.Compare(lhs.FullName, rhs.FullName, StringComparison.Ordinal));
			return settingTypes;
		}

		private static string GetSettingKey(Type type)
		{
			return type.FullName ?? type.Name;
		}

		private void Normalize(string projectPath)
		{
			if (string.IsNullOrWhiteSpace(AssetFolderName))
			{
				AssetFolderName = ResolveDefaultAssetFolderName(projectPath);
			}

			if (string.IsNullOrWhiteSpace(RuntimeRootRelativePath))
			{
				RuntimeRootRelativePath = ResolveDefaultRuntimeRootRelativePath(projectPath);
			}
		}

		private static string ResolveDefaultAssetFolderName(string projectPath)
		{
			foreach (string directoryPath in Directory.EnumerateDirectories(projectPath))
			{
				string directoryName = Path.GetFileName(directoryPath);
				if (string.Equals(directoryName, "assets", StringComparison.OrdinalIgnoreCase))
				{
					return directoryName;
				}
			}

			return Workspace.DefaultAssetFolderName;
		}

		private static string ResolveDefaultRuntimeRootRelativePath(string projectPath)
		{
			if (Directory.Exists(Path.Combine(projectPath, "runtime")))
			{
				return "runtime";
			}

			if (Directory.Exists(Path.GetFullPath(Path.Combine(projectPath, "..", "runtime"))))
			{
				return "..\\runtime";
			}

			return "runtime";
		}
		#endregion
}

	public abstract class ProjectSettingBase
	{
		public abstract string Name { get; }
	}

	public abstract class ProjectSettingRuntime : ProjectSettingBase
{

	}

	public abstract class ProjectSettingEditor : ProjectSettingBase
	{
}

	public static partial class ProjectSettingsExtensions
	{

	}
