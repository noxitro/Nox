
using System.Data;

namespace ReflectionGenerator;

	/// <summary>
	/// エントリーポイント
	/// </summary>
	internal static class Entry
	{
		#region 内部クラス定義
		private enum ErrorCode : int
		{
			Success,
			Error
		}
		private enum MainArgs : byte
		{
			Invalid,

			/// <summary>
			/// 解析対象のソースファイルパス
			/// </summary>
			SourceFilePath,

			/// <summary>
			/// 出力先
			/// </summary>
			OutputDir,

			/// <summary>
			/// ソリューションディレクトリ
			/// </summary>
			SolutionPath,

			/// <summary>
			/// ビルド構成
			/// </summary>
			Configuration,

			/// <summary>
			/// プラットフォーム
			/// </summary>
			Platform,

			/// <summary>
			/// ビルド定義名
			/// </summary>
			ConfigurationDefine,

			/// <summary>
			/// プラットフォーム定義名
			/// </summary>
			PlatformDefine,

			/// <summary>
			/// プロジェクトディレクトリ
			/// </summary>
			ProjectDirectory,

			/// <summary>
			/// プロジェクト名
			/// </summary>
			ProjectName,

			/// <summary>
			/// 出力先ディレクトリ
			/// </summary>
			OutputProjectDir,

			MSBuildBinPath,

			EnableNamespaceList,

			/// <summary>
			/// ビルド中間ディレクトリ
			/// </summary>
			IntermediateDir,


			_Max
		}
    #endregion

    #region 非公開メソッド
		internal static int Run(string[] args)
		{
			return MainProcess2();
		}

		private static int MainProcess2()
		{
			Nox.CustomTask.Data data;
			try
			{
				data = Nox.CustomTask.Util.GetData();
			}
			catch (System.Exception e)
			{
				System.Diagnostics.Debug.Assert(false, "カスタムタスクで収集したバイナリデータの読み込みに失敗しました\n{0}", e.Message);
				return 1;
			}

			
			//	
			Dictionary<string, IReadOnlyList<string>> includeHeaderWithArtifactDict = new();
			{
				//  ソリューションファイルからプロジェクトファイルパスリストを取得する
				IReadOnlyList<string> projectFilePathList = ExtractVCXProjectFile.ExtractBuildOrderProjectPathList(data.SolutionPath);

				foreach (string projectFilePath in projectFilePathList)
				{
					IReadOnlyList<string> headerFiles = ExtractVCXProjectFile.ExtractHeaderFiles(projectFilePath);

					string projectFileName = System.IO.Path.GetFileNameWithoutExtension(projectFilePath);
					includeHeaderWithArtifactDict.Add(projectFileName, headerFiles);
				}
			}

			Parser2.CppParser parser = new Parser2.CppParser();
			if(parser.Parse(new Parser2.CppParser.SetupDesc()
			{ 
				Configuration = data.Configuration,
				Platform = data.Platform,
				SourceFilePath = data.ReflectionTargetSourceFile,
				SolutionPath = data.SolutionPath,
				MSBuildBinPath = data.MSBuildBinPath,
				CppVersion = data.CppVersion,
				Optimization = data.Optimization,
				PreprocessorMacro = data.PreprocessorMacro,
				IgnoreNamespaceList = [],
				EnableRootNamespaceList = [],
				AdditionalIncludeDirectories = data.AdditionalIncludeDirectories,
				ProjectFilePath = data.ProjectPath,
				AdditionalOptions = data.AdditionalOptions,
				UseRTTI = data.UseRtti,
				IncludeHeaderListWithArtifact = includeHeaderWithArtifactDict,
				IntermediateOutputPath = data.IntermediateOutputPath,


			}) == false)
			{
				Trace.Error(null, "解析に失敗しました");
				return 1;
			}


			List<Generator.Generator.ARTIFACT_INFO> moduleInfoList = new List<Generator.Generator.ARTIFACT_INFO>();
			//if (parser.NamespaceDeclListWithProjectName.ContainsKey(Define.UNKNOWN_MODULE_NAME))
			{
				moduleInfoList.Add(new Generator.Generator.ARTIFACT_INFO()
				{
					Build = true,
					ReBuild = false,
					IsModule = false,
					RelativePath = string.Empty,
					ArtifactName = Define.UNKNOWN_MODULE_NAME,
				});
			}

			{
				ReadOnlySpan<string> ignoreProjectList = [
					"runtime"
					];

				ReadOnlySpan<string> ignoreModuleList = [
						"unknown",
						"kernel",
						"core",
						"reflection",
						"reflection_generated",
						];

				foreach (string projectPath in GetProjectPathsFromSlnx(data.SolutionPath))
				{
					string fileNameWithoutExtension = System.IO.Path.GetFileNameWithoutExtension(projectPath);
					if (ignoreProjectList.Contains(fileNameWithoutExtension))
					{
						continue;
					}

					//	ソリューションディレクトリからの相対パス
					string relative = System.IO.Path.GetRelativePath(data.SolutionDir, projectPath);
					relative = relative.Replace("vcxproj", "h");
					relative = relative.Replace("\\", "/");

					moduleInfoList.Add(new Generator.Generator.ARTIFACT_INFO()
					{
						Build = false,
						ReBuild = false,
						ArtifactName = fileNameWithoutExtension,
						RelativePath = relative,
						IsModule = !ignoreModuleList.Contains(fileNameWithoutExtension),
					});
				}
			}

			//	コード出力
			Generator.Generator generator = new Generator.Generator()
			{
				TypeInfoListWithArtifactNameDict = parser.NamespaceDeclListWithProjectNameDict,
				OutputProjectDirectory = data.ProjectDir,
				ModuleInfoList = moduleInfoList.ToArray(),
				Configuration = data.Configuration,
				ConfigurationDefine = data.ConfigurationDefine,
				OutputDirectory = data.OutputGenerateDir,
				Platform = data.Platform,
				PlatformDefine = data.PlatformDefine,

			};

			bool generateSucceeded;
			using (new ScopeProfiler() { Tag = "Generate" })
			{
				generateSucceeded = generator.Generate();
			}

			if (generateSucceeded == false)
			{
				Trace.ErrorLine(null, "コード生成に失敗しました");
				return 1;
			}

			//	ツールで参照するためのバイナリファイルを出力
			using (new ScopeProfiler() { Tag = "Serialize" })
			{
				RuntimeTypeDBHelper.Serialize(data, parser.NamespaceDeclList);
			}

			return 0;
		}

		public static IReadOnlyList<string> GetProjectPathsFromSlnx(string slnxPath)
		{
			if (!File.Exists(slnxPath))
			{
				throw new FileNotFoundException("solution file not found.", slnxPath);
			}

			if (string.Equals(Path.GetExtension(slnxPath), ".sln", StringComparison.OrdinalIgnoreCase))
			{
				return ExtractVCXProjectFile.ExtractBuildOrderProjectPathList(slnxPath);
			}

			System.Xml.Linq.XDocument doc = System.Xml.Linq.XDocument.Load(slnxPath);

			System.Xml.Linq.XElement? solutionElem = doc.Root;
			if (solutionElem is null || solutionElem.Name != "Solution")
			{
				throw new InvalidDataException("Invalid slnx format: root <Solution> not found.");
			}

			string slnxDir = Path.GetDirectoryName(Path.GetFullPath(slnxPath)) ?? string.Empty;

			List<string> projectPaths = new();

			foreach (System.Xml.Linq.XElement projectElem in solutionElem.Descendants("Project"))
			{
				string? rel = projectElem.Attribute("Path")?.Value;
				if (string.IsNullOrWhiteSpace(rel))
				{
					continue;
				}

				string full = Path.GetFullPath(Path.Combine(slnxDir, rel));
				projectPaths.Add(full);
			}

			return projectPaths;
		}
		#endregion
	}
