
using System.Data;

namespace ReflectionGenerator
{
	/// <summary>
	/// エントリーポイント
	/// </summary>
	file class Entry
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
		internal static int Main(string[] args)
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
				System.Diagnostics.Debug.Assert(false, "カスタムタスクで収集したバイナリデータの読み込みに失敗しました");
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
				Trace.Error(null, "解析に失敗しました。");
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
					ArtifactName = Define.UNKNOWN_MODULE_NAME,
					IncludeHeaderList = []
				});
			}

			//	ビルドタイムスタンプファイルを解析
			//      List<string> allModuleNameList = new List<string>();
			//		List<string> targetModuleNameList = new List<string>();
			{
				//	エンジン側のプロジェクトのビルドタイムスタンプファイルのディレクトリを取得
				System.IO.DirectoryInfo? OutputDicretoryInfo = System.IO.Directory.GetParent(data.OutDir);
				if (OutputDicretoryInfo == null)
				{
					Trace.ErrorLine(null, $"出力ディレクトリが見つかりません:{data.OutDir}");
					return 1;
				}

				string engineBuildTimeStampFileDirectory = $"{OutputDicretoryInfo.FullName}\\nox_build_time_stamp";
				engineBuildTimeStampFileDirectory = System.IO.Path.GetFullPath(engineBuildTimeStampFileDirectory);

				if (System.IO.Directory.Exists(engineBuildTimeStampFileDirectory) == false)
				{
					Trace.ErrorLine(null, $"ビルドタイムスタンプファイルのディレクトリが見つかりません:{engineBuildTimeStampFileDirectory}");
					return 1;
				}

				//	   リフレクション生成のタイムスタンプファイルのディレクトリを取得
				string reflectionGenTimeStampDirectory = $"{OutputDicretoryInfo.FullName}\\nox_parse_build_time_stamp";
				reflectionGenTimeStampDirectory = System.IO.Path.GetFullPath(reflectionGenTimeStampDirectory);

				if (System.IO.Directory.Exists(reflectionGenTimeStampDirectory) == false)
				{
					System.IO.Directory.CreateDirectory(reflectionGenTimeStampDirectory);
				}

				foreach (string engineTimeStampFileName in System.IO.Directory.GetFiles(engineBuildTimeStampFileDirectory))
				{
					bool build = false;
					//                    allModuleNameList.Add(engineTimeStampFileName);

					string fileNameWithoutExtension = System.IO.Path.GetFileNameWithoutExtension(engineTimeStampFileName);

					//	リフレクション生成のタイムスタンプファイルのパス
					string reflectionGenTimeStampFilePath = $"{reflectionGenTimeStampDirectory}\\{System.IO.Path.GetFileName(engineTimeStampFileName)}";
					if (System.IO.File.Exists(reflectionGenTimeStampFilePath) == true)
					{
						//	ファイル内のテキストは、
						//	1:	エンジン側のビルドタイムスタンプ
						//	2:	リビルドフラグ

						//	タイムスタンプを比較
						string engineTimeStamp = System.IO.File.ReadAllText(engineTimeStampFileName);
						System.DateTime engineTimeStampDataTime = System.DateTime.Parse(engineTimeStamp);

						string reflectionGenTimeStamp = System.IO.File.ReadAllText(reflectionGenTimeStampFilePath);
						if (System.DateTime.TryParse(reflectionGenTimeStamp, out System.DateTime reflectionGenTimeStampDataTime) == true)
						{
							if (engineTimeStampDataTime > reflectionGenTimeStampDataTime)
							{
								build = true;
								//                          targetModuleNameList.Add(fileNameWithoutExtension);
							}
						}
						else
						{
							//	
							Trace.Warning(null, "リフレクション生成のタイムスタンプファイルのフォーマットが不正です。");
						}
					}
					else
					{
						build = true;
						//                        targetModuleNameList.Add(fileNameWithoutExtension);
					}

					//IReadOnlyList<string> includeHeaderList;
					//if (includeHeaderWithArtifactDict.TryGetValue(fileNameWithoutExtension, out IReadOnlyList<string>? tmpList) == true && tmpList != null)
					//{
					//	includeHeaderList = tmpList;
					//}
					//else
					//{
					//	includeHeaderList = [];
					//}
					System.IO.BinaryReader s;
					System.

					ReadOnlySpan<string> ignoreModuleList = [
						"unknown",
						"kernel",
						"core",
						"reflection",
						"reflection_generated",
						];
					
					moduleInfoList.Add(new Generator.Generator.ARTIFACT_INFO()
					{
						Build = build,
						ReBuild = false,
						ArtifactName = fileNameWithoutExtension,
						IsModule = !ignoreModuleList.Contains(fileNameWithoutExtension),
						IncludeHeaderList = includeHeaderWithArtifactDict[fileNameWithoutExtension]
					}
					);

					//	タイムスタンプを更新
					using (System.IO.StreamWriter streamWriter = new System.IO.StreamWriter(reflectionGenTimeStampFilePath, false, System.Text.Encoding.UTF8))
					{
						streamWriter.WriteLine(System.DateTime.Now.ToString());
					}
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

			using (new ScopeProfiler() { Tag = "Generate" })
			{
				generator.Generate();
			}

			//	ツールで参照するためのバイナリファイルを出力
			using (new ScopeProfiler() { Tag = "Serialize" })
			{
				RuntimeTypeDBHelper.Serialize(data, parser.NamespaceDeclList);
			}

			return 0;
		}
		#endregion
	}
}