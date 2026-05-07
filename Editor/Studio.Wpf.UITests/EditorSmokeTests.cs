using System.Diagnostics;
using System.Drawing;
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
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Hierarchy.RootAddButton"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.AssetBrowser.SearchBox"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.AssetBrowser.AssetList"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Inspector.PropertyList"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.Trace.List"));

		string metaPath = workspace.GetAssetPath("SmokeAsset.txt.meta");
		Assert.True(File.Exists(metaPath), $"Expected asset meta file to be generated: {metaPath}");
		string metaJson = File.ReadAllText(metaPath);
		Assert.Contains("\"Guid\"", metaJson, StringComparison.Ordinal);
		Assert.Contains("\"Kind\": \"Document\"", metaJson, StringComparison.Ordinal);
		Assert.Contains("\"Importer\": \"TextImporter\"", metaJson, StringComparison.Ordinal);
	}

	[Fact]
	public void HierarchyRootAddSelectsNewEntityNodeInInspector()
	{
		using TestWorkspace workspace = TestWorkspace.Create();
		using EditorApp editor = EditorApp.Launch(workspace.RootPath);

		FindByAutomationId(editor.MainWindow, "NoxStudio.Hierarchy.RootAddButton").AsButton().Invoke();

		AutomationElement titleText = WaitForDescendantByName(editor.MainWindow, "EntityNode 4");
		Assert.Equal(ControlType.Text, titleText.ControlType);
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
	public void ProjectSettingsOpensFromViewsMenu()
	{
		using TestWorkspace workspace = TestWorkspace.Create();
		using EditorApp editor = EditorApp.Launch(workspace.RootPath);

		WaitForDescendantByName(editor.MainWindow, "Views").Click();
		RetryResult<AutomationElement?> coreMenuItemResult = Retry.WhileNull(
			() => editor.Automation.GetDesktop().FindFirstDescendant(cf => cf.ByName("Core")),
			UiTimeout);
		AutomationElement coreMenuItem = coreMenuItemResult.Result ?? throw new InvalidOperationException("Core view menu item was not shown.");
		coreMenuItem.Click();

		RetryResult<AutomationElement?> menuItemResult = Retry.WhileNull(
			() => editor.Automation.GetDesktop().FindFirstDescendant(cf => cf.ByName("Project Settings")),
			UiTimeout);
		AutomationElement menuItem = menuItemResult.Result ?? throw new InvalidOperationException("Project Settings menu item was not shown.");
		menuItem.Click();

		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.ProjectSettingsView"));
		Assert.NotNull(FindByAutomationId(editor.MainWindow, "NoxStudio.ProjectSettings.SaveButton"));
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
	public void RebootButtonLaunchesRuntimeAndHostsWindowInRuntimeView()
	{
		string repositoryRoot = EditorApp.ResolveRepositoryRoot();
		string runtimeExecutablePath = Path.Combine(repositoryRoot, "runtime", "build", "runtime", "x64", "Debug", "runtime.exe");
		Assert.True(File.Exists(runtimeExecutablePath), $"runtime.exe was not found. Build runtime before running this FlaUI test: {runtimeExecutablePath}");

		try
		{
			using EditorApp editor = EditorApp.Launch(repositoryRoot);

			FindByAutomationId(editor.MainWindow, "NoxStudio.RuntimeControl.RebootButton").AsButton().Invoke();

			WaitForAttachStatus(editor, "True", runtimeExecutablePath);
			Assert.False(editor.HasExited, "Studio exited after invoking the Reboot button.");
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
}
