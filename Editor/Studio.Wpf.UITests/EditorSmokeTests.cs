using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Runtime.Serialization;
using FlaUI.Core;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.Definitions;
using FlaUI.Core.Tools;
using FlaUI.UIA3;
using Xunit;

[assembly: CollectionBehavior(DisableTestParallelization = true)]

namespace Studio.Wpf.UITests;

public sealed class EditorSmokeTests
{
	private static readonly TimeSpan UiTimeout = TimeSpan.FromSeconds(20);

	[Fact]
	public void EditorLaunchesWithMainPanelsAndGeneratesAssetMeta()
	{
		using TestWorkspace workspace = TestWorkspace.Create();
		workspace.WriteAsset("SmokeAsset.txt", "Nox UI smoke asset");

		using EditorApp editor = EditorApp.Launch(workspace.RootPath);

		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.RuntimeControlView"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Hierarchy.Tree"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.AssetBrowser.SearchBox"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.AssetBrowser.AssetList"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.PropertyInspector.PropertyList"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Trace.LevelFilter"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Trace.ChannelTabs"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Trace.SearchBox"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Trace.List"));
		WaitForDescendantByName(editor.MainWindow, "File").Click();
		Assert.NotNull(FindDesktopByAutomationId(editor, "NoxStudio.File.NewScene"));
		Assert.NotNull(FindDesktopByAutomationId(editor, "NoxStudio.File.SaveScene"));
		Assert.NotNull(FindDesktopByAutomationId(editor, "NoxStudio.File.SaveAs"));
		editor.MainWindow.Click();

		string metaPath = workspace.GetAssetPath("SmokeAsset.txt.meta");
		Assert.True(File.Exists(metaPath), $"Expected asset meta file to be generated: {metaPath}");
		string metaJson = File.ReadAllText(metaPath);
		Assert.Contains("\"Guid\"", metaJson, StringComparison.Ordinal);
		Assert.DoesNotContain("\"Kind\"", metaJson, StringComparison.Ordinal);
		Assert.Contains("\"Importer\": \"TextImporter\"", metaJson, StringComparison.Ordinal);
	}

	[Fact]
	public void HierarchyRootAddSelectsNewEntityNodeInInspector()
	{
		using TestWorkspace workspace = TestWorkspace.Create();
		using EditorApp editor = EditorApp.Launch(workspace.RootPath);

		AutomationElement hierarchyTree = FindByAutomationId(editor.MainWindow, "NoxStudio.Hierarchy.Tree");
		Assert.Null(hierarchyTree.FindFirstDescendant(cf => cf.ByName("Main Scene")));
		Assert.NotNull(WaitForDescendantByName(hierarchyTree, "Main Camera"));

		CreateRootEntityFromHierarchyContextMenu(editor);

		AutomationElement titleText = WaitForDescendantByName(editor.MainWindow, "EntityNode 3");
		Assert.Equal(ControlType.Text, titleText.ControlType);

		InvokeTopMenuItem(editor, "File", "NoxStudio.File.SaveScene");
		string scenePath = workspace.GetAssetPath("Main Scene.noxscene");
		RetryResult<string?> sceneFileResult = Retry.WhileNull(
			() => File.Exists(scenePath) ? scenePath : null,
			UiTimeout);
		Assert.NotNull(sceneFileResult.Result);
		string sceneJson = File.ReadAllText(scenePath);
		Assert.Contains("\"Name\": \"Main Scene\"", sceneJson, StringComparison.Ordinal);
		Assert.Contains("\"Name\": \"Main Camera\"", sceneJson, StringComparison.Ordinal);
		Assert.Contains("\"Name\": \"EntityNode 3\"", sceneJson, StringComparison.Ordinal);
	}

	[Fact]
	public void AssetBrowserSearchFindsAssetsUnderAssetsFolder()
	{
		using TestWorkspace workspace = TestWorkspace.Create();
		workspace.WriteAsset("Textures\\FlaUITexture.txt", "texture placeholder");
		workspace.WriteProjectFileOutsideAssets("OutsideAsset.txt", "must not be shown in Asset Browser");

		using EditorApp editor = EditorApp.Launch(workspace.RootPath);

		FindByAutomationId(editor.MainWindow, "NoxStudio.AssetBrowser.SearchBox").AsTextBox().Text = "FlaUITexture";

		Assert.NotNull(WaitForDescendantByName(editor.MainWindow, "FlaUITexture.txt"));
		Assert.Null(editor.MainWindow.FindFirstDescendant(cf => cf.ByName("OutsideAsset.txt")));
	}

	[Fact]
	public void ProjectSettingsOpensFromProjectMenu()
	{
		using TestWorkspace workspace = TestWorkspace.Create();
		using EditorApp editor = EditorApp.Launch(workspace.RootPath);

		WaitForDescendantByName(editor.MainWindow, "Project").Click();

		RetryResult<AutomationElement?> menuItemResult = Retry.WhileNull(
			() => editor.Automation.GetDesktop().FindFirstDescendant(cf => cf.ByName("Project Settings")),
			UiTimeout);
		AutomationElement menuItem = menuItemResult.Result ?? throw new InvalidOperationException("Project Settings menu item was not shown.");
		menuItem.Click();

		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.ProjectSettingsView"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.ProjectSettings.SaveButton"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.ProjectSettings.PropertyInspector"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.PropertyInspector.TextField"));
	}

	[Fact]
	public void AvalonDockPaneActivationChangesHeaderColor()
	{
		using TestWorkspace workspace = TestWorkspace.Create();
		using EditorApp editor = EditorApp.Launch(workspace.RootPath);

		AutomationElement hierarchyView = FindByAutomationId(editor.MainWindow, "NoxStudio.HierarchyView");
		AutomationElement inspectorView = FindByAutomationId(editor.MainWindow, "NoxStudio.InspectorView");

		Color hierarchyInactiveColor = CaptureAverageColor(editor.MainWindow, GetHeaderSampleRectangle(hierarchyView));
		inspectorView.Click();
		Thread.Sleep(300);
		Color inspectorActiveColor = CaptureAverageColor(editor.MainWindow, GetHeaderSampleRectangle(inspectorView));

		hierarchyView.Click();
		Thread.Sleep(300);
		Color hierarchyActiveColor = CaptureAverageColor(editor.MainWindow, GetHeaderSampleRectangle(hierarchyView));

		Assert.True(ColorDistance(hierarchyInactiveColor, hierarchyActiveColor) >= 20,
			$"Hierarchy header color did not change enough. Inactive={hierarchyInactiveColor}, Active={hierarchyActiveColor}, InspectorActive={inspectorActiveColor}");
		Assert.True(ColorDistance(inspectorActiveColor, hierarchyActiveColor) < 90,
			$"Active pane colors are not similar. InspectorActive={inspectorActiveColor}, HierarchyActive={hierarchyActiveColor}");
	}

	[Fact]
	public void ThemeMenuSwitchesAllThemesWithoutBreakingMainPanels()
	{
		using TestWorkspace workspace = TestWorkspace.Create();
		using EditorApp editor = EditorApp.Launch(workspace.RootPath);

		foreach ((string themeKey, string themeName) in new[] { ("Gunmetal", "Gunmetal"), ("BrushedSteel", "Brushed Steel"), ("Bronze", "Bronze"), ("Monochrome", "Monochrome"), ("Nox", "Nox") })
		{
			InvokeThemeMenuItem(editor, themeKey);

			Assert.False(editor.HasExited, $"Editor exited after applying theme '{themeName}'.");
			Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Hierarchy.Tree"));
			Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.PropertyInspector.PropertyList"));
			Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Trace.List"));
		}
	}

	[Fact]
	public void RuntimeObjectDirtyFlagsTrackStringAndBooleanEdits()
	{
		FakeInspectorRuntimeObject runtimeObject = new();

		Assert.False(runtimeObject.IsDirty("Enabled"));
		Assert.False(runtimeObject.IsDirty("Title"));

		runtimeObject.SetValue("Enabled", true);
		runtimeObject.SetValue("Title", "Player");

		Assert.True(runtimeObject.IsDirty("Enabled"));
		Assert.True(runtimeObject.IsDirty("Title"));

		runtimeObject.ClearDirtyFlags();

		Assert.False(runtimeObject.IsDirty("Enabled"));
		Assert.False(runtimeObject.IsDirty("Title"));
	}

	[Fact]
	public void RebootButtonLaunchesRuntimeAndHostsWindowInRuntimeView()
	{
		string repositoryRoot = EditorApp.ResolveRepositoryRoot();
		string runtimeExecutablePath = Path.Combine(repositoryRoot, "runtime", "build", "runtime", "x64", "Debug", "runtime.exe");
		Assert.True(File.Exists(runtimeExecutablePath), $"runtime.exe was not found. Build runtime before running this FlaUI test: {runtimeExecutablePath}");

		try
		{
			using EditorApp editor = EditorApp.Launch(repositoryRoot);

			FindByAutomationId(editor.MainWindow, "NoxStudio.RuntimeControl.RebootButton").AsButton().Invoke();

			AutomationElement attachStatus = WaitForAttachStatus(editor, "True", runtimeExecutablePath);
			int sentBeforeAdd = GetDebugCounter(attachStatus.HelpText, "Sent");
			int deserializedBeforeAdd = GetDebugCounter(attachStatus.HelpText, "Deserialized");
			CreateRootEntityFromHierarchyContextMenu(editor);
			Assert.NotNull(WaitForDescendantByName(editor.MainWindow, "EntityNode 3"));
			WaitForDebugCounterAtLeast(editor, "Sent", sentBeforeAdd + 1, runtimeExecutablePath);
			WaitForDebugCounterAtLeast(editor, "Deserialized", deserializedBeforeAdd + 1, runtimeExecutablePath);
			int sentAfterCreate = GetDebugCounter(
				FindByAutomationId(editor.MainWindow, "NoxStudio.RuntimeView.AttachStatus").HelpText,
				"Sent");
			int deserializedAfterCreate = GetDebugCounter(
				FindByAutomationId(editor.MainWindow, "NoxStudio.RuntimeView.AttachStatus").HelpText,
				"Deserialized");
			WaitForDebugCounterAtLeast(editor, "Sent", sentAfterCreate + 1, runtimeExecutablePath);
			WaitForDebugCounterAtLeast(editor, "Deserialized", deserializedAfterCreate + 1, runtimeExecutablePath);
			Assert.False(editor.HasExited, "Studio exited after invoking the Reboot button.");
		}
		finally
		{
			KillRuntimeProcesses(runtimeExecutablePath);
		}
	}

	[Fact]
	public void TransformInspectorFieldsStaySyncedAfterRuntimeRoundTrip()
	{
		string repositoryRoot = EditorApp.ResolveRepositoryRoot();
		string runtimeExecutablePath = Path.Combine(repositoryRoot, "runtime", "build", "runtime", "x64", "Debug", "runtime.exe");
		Assert.True(File.Exists(runtimeExecutablePath), $"runtime.exe was not found. Build runtime before running this FlaUI test: {runtimeExecutablePath}");

		try
		{
			using EditorApp editor = EditorApp.Launch(repositoryRoot);

			FindByAutomationId(editor.MainWindow, "NoxStudio.RuntimeControl.RebootButton").AsButton().Invoke();
			AutomationElement attachStatus = WaitForAttachStatus(editor, "True", runtimeExecutablePath);
			int sentBeforeEdit = GetDebugCounter(attachStatus.HelpText, "Sent");
			int deserializedBeforeEdit = GetDebugCounter(attachStatus.HelpText, "Deserialized");

			AutomationElement hierarchyTree = FindByAutomationId(editor.MainWindow, "NoxStudio.Hierarchy.Tree");
			WaitForDescendantByName(hierarchyTree, "Main Camera").Click();

			AutomationElement componentTree = FindByAutomationId(editor.MainWindow, "NoxStudio.Inspector.ComponentTree");
			Assert.NotNull(WaitForDescendantByName(componentTree, "Transform"));
			Assert.NotNull(WaitForDescendantByName(componentTree, "LocalPosition"));
			Assert.NotNull(WaitForDescendantByName(componentTree, "LocalScale"));
			Assert.NotNull(WaitForDescendantByName(componentTree, "LocalRotation"));

			RetryResult<TextBox[]?> textBoxResult = Retry.WhileNull(
				() =>
				{
					TextBox[] edits = componentTree.FindAllDescendants(cf => cf.ByControlType(ControlType.Edit))
						.Select(element => element.AsTextBox())
						.ToArray();
					return edits.Length >= 3 ? edits : null;
				},
				UiTimeout);
			TextBox[] textBoxes = textBoxResult.Result ?? throw new InvalidOperationException(
				"Transform vector text boxes were not found. " + DescribeAutomationSubtree(componentTree));
			Assert.True(textBoxes.Length >= 3, "Expected LocalPosition to expose three editable text boxes.");

			textBoxes[0].Click();
			textBoxes[0].Text = "123";
			textBoxes[1].Click();

			RetryResult<bool> valueResult = Retry.WhileFalse(
				() => textBoxes[0].Text.StartsWith("123", StringComparison.Ordinal),
				TimeSpan.FromSeconds(3));
			Assert.True(valueResult.Success, $"LocalPosition X did not remain synchronized after edit. Actual='{textBoxes[0].Text}'.");

			WaitForDebugCounterAtLeast(editor, "Sent", sentBeforeEdit + 1, runtimeExecutablePath);
			WaitForDebugCounterAtLeast(editor, "Deserialized", deserializedBeforeEdit + 1, runtimeExecutablePath);
		}
		finally
		{
			KillRuntimeProcesses(runtimeExecutablePath);
		}
	}

	private static AutomationElement FindByAutomationId(AutomationElement root, string automationId)
	{
		RetryResult<AutomationElement?> result = Retry.WhileNull(
			() => root.FindFirstDescendant(cf => cf.ByAutomationId(automationId)),
			UiTimeout);
		return result.Result ?? throw new InvalidOperationException($"Element not found. AutomationId={automationId}");
	}

	private static AutomationElement WaitForDescendantByName(AutomationElement root, string name)
	{
		RetryResult<AutomationElement?> result = Retry.WhileNull(
			() => root.FindFirstDescendant(cf => cf.ByName(name)),
			UiTimeout);
		return result.Result ?? throw new InvalidOperationException($"Element not found. Name={name}");
	}

	private static void CreateRootEntityFromHierarchyContextMenu(EditorApp editor)
	{
		AutomationElement hierarchyTree = FindByAutomationId(editor.MainWindow, "NoxStudio.Hierarchy.Tree");
		hierarchyTree.RightClick();
		AutomationElement createEntity = FindDesktopByAutomationId(editor, "NoxStudio.Hierarchy.Context.CreateEntity");
		createEntity.Click();
	}

	private static void InvokeTopMenuItem(EditorApp editor, string topLevelName, string menuItemAutomationId)
	{
		WaitForDescendantByName(editor.MainWindow, topLevelName).Click();
		FindDesktopByAutomationId(editor, menuItemAutomationId).Click();
	}

	private static void InvokeThemeMenuItem(EditorApp editor, string themeKey)
	{
		editor.MainWindow.Click();
		Thread.Sleep(100);
		WaitForDescendantByName(editor.MainWindow, "View").AsMenuItem().Expand();
		WaitForDesktopElementByName(editor, "Theme").AsMenuItem().Expand();
		FindDesktopByAutomationId(editor, $"NoxStudio.Theme.{themeKey}").Click();
		Thread.Sleep(100);
	}

	private static AutomationElement WaitForDesktopElementByName(EditorApp editor, string name)
	{
		RetryResult<AutomationElement?> result = Retry.WhileNull(
			() => editor.Automation.GetDesktop().FindFirstDescendant(cf => cf.ByName(name)),
			UiTimeout);
		return result.Result ?? throw new InvalidOperationException($"Desktop element not found. Name={name}");
	}

	private static AutomationElement FindDesktopByAutomationId(EditorApp editor, string automationId)
	{
		RetryResult<AutomationElement?> result = Retry.WhileNull(
			() => editor.Automation.GetDesktop().FindFirstDescendant(cf => cf.ByAutomationId(automationId)),
			UiTimeout);
		return result.Result ?? throw new InvalidOperationException($"Desktop element not found. AutomationId={automationId}");
	}

	private static AutomationElement WaitForAttachStatus(EditorApp editor, string expectedText, string runtimeExecutablePath)
	{
		RetryResult<AutomationElement?> result = Retry.WhileNull(
			() =>
			{
				AutomationElement root = editor.MainWindow;
				AutomationElement? status = root.FindFirstDescendant(cf => cf.ByAutomationId("NoxStudio.RuntimeView.AttachStatus"));
				return status?.Name.Contains(expectedText, StringComparison.Ordinal) == true ? status : null;
			},
			UiTimeout);

		return result.Result ?? throw new InvalidOperationException(
			$"Runtime view was not attached. Expected status text '{expectedText}'. EditorExited={editor.HasExited}. {GetRuntimeViewDiagnostics(editor.MainWindow)} {GetRuntimeDiagnostics(runtimeExecutablePath)}");
	}

	private static void WaitForDebugCounterAtLeast(EditorApp editor, string counterName, int expectedMinimum, string runtimeExecutablePath)
	{
		RetryResult<AutomationElement?> result = Retry.WhileNull(
			() =>
			{
				AutomationElement status = FindByAutomationId(editor.MainWindow, "NoxStudio.RuntimeView.AttachStatus");
				return GetDebugCounter(status.HelpText, counterName) >= expectedMinimum ? status : null;
			},
			UiTimeout);

		Assert.NotNull(result.Result);
		Assert.False(editor.HasExited, $"Editor exited while waiting for RuntimeView debug counter {counterName}>={expectedMinimum}. {GetRuntimeViewDiagnostics(editor.MainWindow)} {GetRuntimeDiagnostics(runtimeExecutablePath)}");
	}

	private static int GetDebugCounter(string? debugText, string counterName)
	{
		if (string.IsNullOrWhiteSpace(debugText))
		{
			return 0;
		}

		string prefix = counterName + "=";
		foreach (string part in debugText.Split(';', StringSplitOptions.TrimEntries))
		{
			if (part.StartsWith(prefix, StringComparison.Ordinal) &&
				int.TryParse(part.AsSpan(prefix.Length), out int value))
			{
				return value;
			}
		}

		return 0;
	}

	private static Rectangle GetHeaderSampleRectangle(AutomationElement view)
	{
        Rectangle bounds = view.BoundingRectangle;
		return new Rectangle(
           bounds.Left + 8,
			Math.Max(0, bounds.Top - 19),
			Math.Max(1, Math.Min(120, bounds.Width - 16)),
			14);
	}

	private static Color CaptureAverageColor(Window mainWindow, Rectangle sampleRectangle)
	{
		mainWindow.Focus();
		using Bitmap bitmap = new(sampleRectangle.Width, sampleRectangle.Height);
		using Graphics graphics = Graphics.FromImage(bitmap);
		graphics.CopyFromScreen(sampleRectangle.Location, Point.Empty, sampleRectangle.Size);

		long r = 0;
		long g = 0;
		long b = 0;
		int count = 0;

		for (int y = 0; y < bitmap.Height; y++)
		{
			for (int x = 0; x < bitmap.Width; x++)
			{
				Color color = bitmap.GetPixel(x, y);
				r += color.R;
				g += color.G;
				b += color.B;
				count++;
			}
		}

		return Color.FromArgb((int)(r / count), (int)(g / count), (int)(b / count));
	}

	private static double ColorDistance(Color left, Color right)
	{
		int r = left.R - right.R;
		int g = left.G - right.G;
		int b = left.B - right.B;
		return Math.Sqrt((r * r) + (g * g) + (b * b));
	}

	private static string GetRuntimeViewDiagnostics(AutomationElement root)
	{
		AutomationElement? statusElement = root.FindFirstDescendant(cf => cf.ByAutomationId("NoxStudio.RuntimeView.AttachStatus"));
		string attachStatus = statusElement?.Name ?? "<missing>";
		string debugStatus = statusElement?.HelpText ?? "<missing>";

		return $"RuntimeView AttachStatus='{attachStatus}', DebugStatus='{debugStatus}'.";
	}

	private static string GetRuntimeDiagnostics(string runtimeExecutablePath)
	{
		List<string> diagnostics = new();
		foreach (Process process in Process.GetProcessesByName("runtime"))
		{
			try
			{
				string? path = null;
				try
				{
					path = process.MainModule?.FileName;
				}
				catch (InvalidOperationException)
				{
				}
				catch (System.ComponentModel.Win32Exception)
				{
				}
				catch (NotSupportedException)
				{
				}

				if (path == null || string.Equals(path, runtimeExecutablePath, StringComparison.OrdinalIgnoreCase))
				{
					diagnostics.Add($"PID={process.Id}, Exited={process.HasExited}, MainWindowHandle=0x{process.MainWindowHandle.ToInt64():X}, Title='{process.MainWindowTitle}', Windows={GetProcessWindowDiagnostics(process)}");
				}
			}
			finally
			{
				process.Dispose();
			}
		}

		return diagnostics.Count == 0 ? "No matching runtime.exe process was found." : string.Join("; ", diagnostics);
	}

	private static string DescribeAutomationSubtree(AutomationElement root)
	{
		return string.Join(" | ", root.FindAllDescendants().Take(80).Select(element =>
			$"Name='{element.Name}', Id='{element.AutomationId}', Type='{element.ControlType}'"));
	}

	private static string GetProcessWindowDiagnostics(Process process)
	{
		try
		{
			using UIA3Automation automation = new();
			Application application = Application.Attach(process);
			Window[] windows = application.GetAllTopLevelWindows(automation);
			return string.Join(" | ", windows.Select(window =>
				$"Window='{window.Name}' Children='{string.Join(", ", window.FindAllDescendants().Take(12).Select(child => child.Name).Where(name => string.IsNullOrWhiteSpace(name) == false))}'"));
		}
		catch (Exception ex)
		{
			return $"<window diagnostics failed: {ex.GetType().Name}: {ex.Message}>";
		}
	}

	private static void KillRuntimeProcesses(string runtimeExecutablePath)
	{
		foreach (Process process in Process.GetProcessesByName("runtime"))
		{
			try
			{
				string? path = null;
				try
				{
					path = process.MainModule?.FileName;
				}
				catch (InvalidOperationException)
				{
					continue;
				}
				catch (System.ComponentModel.Win32Exception)
				{
					continue;
				}
				catch (NotSupportedException)
				{
					continue;
				}

				if (string.Equals(path, runtimeExecutablePath, StringComparison.OrdinalIgnoreCase) && process.HasExited == false)
				{
					process.Kill(entireProcessTree: true);
					process.WaitForExit(5000);
				}
			}
			finally
			{
				process.Dispose();
			}
		}
	}

	private sealed class EditorApp : IDisposable
	{
		private readonly Application _Application;
		private readonly UIA3Automation _Automation;
		private bool _Disposed;

		public Window MainWindow { get; }
		public UIA3Automation Automation => _Automation;
		public bool HasExited => _Application.HasExited;

		private EditorApp(Application application, UIA3Automation automation, Window mainWindow)
		{
			_Application = application;
			_Automation = automation;
			MainWindow = mainWindow;
		}

		public static EditorApp Launch(string workspacePath)
		{
			string editorExecutablePath = ResolveEditorExecutablePath();
			ProcessStartInfo startInfo = new()
			{
				FileName = editorExecutablePath,
				Arguments = $"--EngineRootDir=\"{workspacePath}\"",
				WorkingDirectory = Path.GetDirectoryName(editorExecutablePath)!,
				UseShellExecute = false,
			};

			Application application = Application.Launch(startInfo);
			UIA3Automation automation = new();
			RetryResult<Window?> mainWindowResult = Retry.WhileNull(
				() => application.GetMainWindow(automation),
				UiTimeout);
			Window mainWindow = mainWindowResult.Result ?? throw new InvalidOperationException("Nox Studio main window was not created.");
			return new EditorApp(application, automation, mainWindow);
		}

		public void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			try
			{
				if (_Application.HasExited == false)
				{
					_Application.Close();
				}
			}
			finally
			{
				if (_Application.HasExited == false)
				{
					_Application.Kill();
				}

				_Automation.Dispose();
				_Disposed = true;
			}
		}

		private static string ResolveEditorExecutablePath()
		{
			string repositoryRoot = ResolveRepositoryRoot();
			string configuration = ResolveBuildConfiguration();
			string executablePath = Path.Combine(repositoryRoot, "Editor", "Studio.Wpf", "bin", configuration, "net10.0-windows", "Studio.Wpf.exe");
			if (File.Exists(executablePath))
			{
				return executablePath;
			}

			string debugExecutablePath = Path.Combine(repositoryRoot, "Editor", "Studio.Wpf", "bin", "Debug", "net10.0-windows", "Studio.Wpf.exe");
			if (File.Exists(debugExecutablePath))
			{
				return debugExecutablePath;
			}

			throw new FileNotFoundException("Studio.Wpf.exe was not found. Build Studio.Wpf before running UI tests.", executablePath);
		}

		public static string ResolveRepositoryRoot()
		{
			DirectoryInfo? directory = new(AppContext.BaseDirectory);
			while (directory != null)
			{
				if (File.Exists(Path.Combine(directory.FullName, "Editor", "Studio.Wpf", "Studio.Wpf.csproj")))
				{
					return directory.FullName;
				}

				directory = directory.Parent;
			}

			throw new DirectoryNotFoundException("Could not resolve repository root from test output directory.");
		}

		private static string ResolveBuildConfiguration()
		{
			DirectoryInfo outputDirectory = new(AppContext.BaseDirectory);
			return outputDirectory.Parent?.Name ?? "Debug";
		}
	}

	private sealed class TestWorkspace : IDisposable
	{
		public string RootPath { get; }
		private bool _Disposed;

		private TestWorkspace(string rootPath)
		{
			RootPath = rootPath;
			Directory.CreateDirectory(GetAssetPath());
		}

		public static TestWorkspace Create()
		{
			string rootPath = Path.Combine(Path.GetTempPath(), "NoxStudioFlaUITests", Guid.NewGuid().ToString("N"));
			return new TestWorkspace(rootPath);
		}

		public void WriteAsset(string relativePath, string content)
		{
			string path = GetAssetPath(relativePath);
			Directory.CreateDirectory(Path.GetDirectoryName(path)!);
			File.WriteAllText(path, content);
		}

		public void WriteProjectFileOutsideAssets(string relativePath, string content)
		{
			string path = Path.Combine(RootPath, relativePath);
			Directory.CreateDirectory(Path.GetDirectoryName(path)!);
			File.WriteAllText(path, content);
		}

		public string GetAssetPath(string relativePath = "")
		{
			return Path.Combine(RootPath, "Assets", relativePath);
		}

		public void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			try
			{
				if (Directory.Exists(RootPath))
				{
					Directory.Delete(RootPath, recursive: true);
				}
			}
			catch (IOException)
			{
			}
			catch (UnauthorizedAccessException)
			{
			}
			finally
			{
				_Disposed = true;
			}
		}
	}

	[Core.Attributes.RuntimeWrapper("nox::FakeInspectorRuntimeObject")]
	private sealed class FakeInspectorRuntimeObject : Core.RuntimeObject
	{
		static FakeInspectorRuntimeObject()
		{
			Type holderType = typeof(Core.RuntimeObject)
				.GetNestedType("RuntimeRecordDeclHolder`1", BindingFlags.NonPublic)!
				.MakeGenericType(typeof(FakeInspectorRuntimeObject));
			holderType.GetField("Value", BindingFlags.Public | BindingFlags.Static)!.SetValue(null, CreateRuntimeRecordDecl());
		}

		private static Core.RuntimeRecordDecl CreateRuntimeRecordDecl()
		{
			Core.RuntimeTypeInfo boolType = new()
			{
				TypeKind = Core.RuntimeTypeKind.Bool,
				Size = 1,
				Alignment = 1,
				Name = "bool",
				FullName = "bool",
				Namespace = string.Empty,
			};
			Core.RuntimeTypeInfo stringType = new()
			{
				TypeKind = Core.RuntimeTypeKind.Class,
				Size = 24,
				Alignment = 8,
				Name = "string",
				FullName = "System.String",
				Namespace = "System",
			};
			Core.RuntimeTypeInfo recordType = new()
			{
				TypeKind = Core.RuntimeTypeKind.Class,
				Size = 1,
				Alignment = 1,
				Name = nameof(FakeInspectorRuntimeObject),
				FullName = "nox::FakeInspectorRuntimeObject",
				Namespace = "nox",
			};
			DataMemberAttribute remoteAttribute = new();
			return new Core.RuntimeRecordDecl
			{
				Name = nameof(FakeInspectorRuntimeObject),
				FullName = "nox::FakeInspectorRuntimeObject",
				Namespace = "nox",
				AttributeList = [],
				RecordList = [],
				EnumList = [],
				FunctionList = [],
				IsNoxObject = true,
				TypeInfo = recordType,
				VariableList =
				[
					new Core.RuntimeVariableDecl
					{
						Name = "Enabled",
						FullName = "nox::FakeInspectorRuntimeObject::Enabled",
						Namespace = "nox",
						AttributeList = [remoteAttribute],
						TypeInfo = boolType,
						VariableAttributeFlags = Core.RuntimeVariableAttributeFlag.None,
						OffsetBits = 0,
						BitFieldWidth = 0,
					},
					new Core.RuntimeVariableDecl
					{
						Name = "Title",
						FullName = "nox::FakeInspectorRuntimeObject::Title",
						Namespace = "nox",
						AttributeList = [remoteAttribute],
						TypeInfo = stringType,
						VariableAttributeFlags = Core.RuntimeVariableAttributeFlag.None,
						OffsetBits = 8,
						BitFieldWidth = 0,
					},
				],
			};
		}
	}
}
