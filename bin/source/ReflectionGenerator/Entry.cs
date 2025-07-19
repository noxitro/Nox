
using System.Data;

namespace ReflectionGenerator
{
	/// <summary>
	/// エントリーポイント
	/// </summary>
	internal static class Entry
	{
		#region 列挙体定義
		private enum ErrorCode : int
		{
			Success,
			Error
		}
        #endregion

        #region 内部クラス定義

		private class ConfigJsonData
		{

		}

		private class MainArgsData
        {
			/// <summary>
			/// モジュールごとの情報
			/// </summary>
			public class ModuleData
            {
				/// <summary>
				/// ソースファイルパス
				/// </summary>
				public string SourceFilePath { get; set; } = string.Empty;

				/// <summary>
				/// タイムスタンプ
				/// </summary>
				public string TimeStamp { get; set; } = string.Empty; 
            }

			/// <summary>
			/// モジュール情報リスト
			/// </summary>
			public List<ModuleData> ModuleDataList { get; set; } = new List<ModuleData>();

			public string SourceFilePath { get; set; } = string.Empty;

			public string OutputDirectory { get; set; } = string.Empty;

			public string Configuration { get; set; } = string.Empty;
			public string Platform { get; set; } = string.Empty;
			public string ConfigurationDefine { get; set; } = string.Empty;
			public string PlatformDefine { get; set; } = string.Empty;

			public string MSBuildBinPath { get; set; } = string.Empty;

			/// <summary>
			/// 出力先ディレクトリ
			/// </summary>
			public string OutputProjectDir { get; set; } = string.Empty;


			public string SolutionFilePath { get; set; } = string.Empty;

			/// <summary>
			/// ビルド中間ディレクトリ
			/// </summary>
			public string IntermediateDir { get; set; } = string.Empty;

            public List<string> IgnoreNamespaceList { get; set; } = new List<string>();

			public List<string> EnableNamespaceList { get; set; } = new List<string>();
        }
        #endregion

        #region 非公開メソッド
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

		internal static int Main(string[] args)
		{
			//	新制御
#if true
			return MainProcess2();
#else
			//	旧制御
			return MainProcess(args);
#endif

		}

		static string GetP()
		{
			string tempPath = System.IO.Path.GetTempPath();

			string path = System.IO.Path.GetFullPath(tempPath + "/NoxReflectionPreData.bin");

			return path;
		}

		static string GetP2()
		{
			string tempPath = System.IO.Path.GetTempPath();

			string path = System.IO.Path.GetFullPath(tempPath + "/NoxReflectionPreData2.bin");

			return path;
		}

		private static void Test()
		{
			Nox.CustomTask.Data data = new ();
			data.CppVersion = "cpp17";

			string path2 = GetP2();
			string path = GetP();

			WriteToFile(path2, data);


			Nox.CustomTask.Data data2 = new();
			byte[] buffer = System.IO.File.ReadAllBytes(path);
			byte[] buffer2 = System.IO.File.ReadAllBytes(path2);

			unsafe
			{
				fixed (byte* p = buffer)
				{
					System.Runtime.InteropServices.Marshal.PtrToStructure((IntPtr)p, data2);
				}
			}
		}

		private static void WriteToFile<T>(string filePath, T data)
		{
			int size = System.Runtime.InteropServices.Marshal.SizeOf(data);
			byte[] buffer = new byte[size];

			unsafe
			{
				fixed (byte* p = buffer)
				{
					System.Runtime.InteropServices.Marshal.StructureToPtr(data, (IntPtr)p, false);
				}
			}

			using (var fileStream = new System.IO.FileStream(filePath, System.IO.FileMode.Create, System.IO.FileAccess.Write))
			{
				fileStream.Write(buffer, 0, buffer.Length);
			}
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

			Parser.CppParser parser = new Parser.CppParser();
			if(parser.Parse(new Parser.CppParser.SetupDesc()
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
				UseRtti = data.UseRtti,

			}) == false)
			{
				Trace.Error(null, "解析に失敗しました。");
				return 1;
			}

			if (parser.RootDeclHolder == null)
			{
				return 1;
			}

			List<Generator.Generator.ARTIFACT_INFO> moduleInfoList = new List<Generator.Generator.ARTIFACT_INFO>();
			if (parser.TypeInfoListWithModuleNameDict.ContainsKey(Define.UNKNOWN_MODULE_NAME))
			{
				moduleInfoList.Add(new Generator.Generator.ARTIFACT_INFO()
				{
					Build = true,
					ReBuild = false,
					ArtifactName = Define.UNKNOWN_MODULE_NAME,
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

					moduleInfoList.Add(new Generator.Generator.ARTIFACT_INFO()
					{
						Build = build,
						ReBuild = false,
						ArtifactName = fileNameWithoutExtension,
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
				OutputProjectDirectory = data.ProjectDir,
				ModuleInfoList = moduleInfoList.ToArray(),
				//                TargetModuleNameList = targetModuleNameList,
				//AllModuleNameList = allModuleNameList,
				TypeInfoListWithArtifactNameDict = parser.TypeInfoListWithModuleNameDict,
				Configuration = data.Configuration,
				ConfigurationDefine = data.ConfigurationDefine,
				OutputDirectory = data.OutputGenerateDir,
				Platform = data.Platform,
				PlatformDefine = data.PlatformDefine,

			};

			generator.Generate();

			//			generator.Setup(parser, argsData.OutputDirectory, argsData.BuildSpec, argsData.Platform, argsData.BuildSpecDefine, argsData.PlatformDefine);
			//			generator.Generate();

			Console.ReadLine();

			return 0;
		}

		#endregion
	}
}