

using ClangSharp;
using System.Runtime.InteropServices.Marshalling;

namespace ReflectionGenerator
{
	/// <summary>
	/// エントリーポイント
	/// </summary>
	public static class Entry
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
			public string BuildSpecDefine { get; set; } = string.Empty;
			public string PlatformDefine { get; set; } = string.Empty;

			public string MSBuildBinPath { get; set; } = string.Empty;
			public string ProjectFilePath { get; set; } = string.Empty;
            public string SolutionFilePath { get; set; } = string.Empty;

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


			ProjectPath,

			MSBuildBinPath,

            EnableNamespaceList,

			_Max
		}

		/// <summary>
		/// エントリーポイント
		/// </summary>
		/// <param name="args"></param>
		/// <returns></returns>
		private static int Main(string[] args)
		{
#if true   //	Test            
			string reflectionGenerateArgsFilePath = string.Empty;
            string test = "runtime";
		
			//	sample
			if (test == "sample")
			{
				reflectionGenerateArgsFilePath = $"D:\\github\\Nox\\runtime\\bin\\source\\ReflectionGenerator\\reflectionGenerateArgs.txt";
				
			}
			else if (test == "runtime")
			{
                reflectionGenerateArgsFilePath = $"D:\\github\\Nox\\runtime\\build\\reflectionGenerateArgs.txt";
            }

            reflectionGenerateArgsFilePath = System.IO.Path.GetFullPath(reflectionGenerateArgsFilePath);

            if (System.IO.File.Exists(reflectionGenerateArgsFilePath) == true)
            {
                args = System.IO.File.ReadAllLines(reflectionGenerateArgsFilePath);
            }
#endif

            Dictionary<string, MainArgs> mainArgTypeDict = new Dictionary<string, MainArgs>();
            {
                MainArgs[] mainArgValueList = Enum.GetValues<MainArgs>();
				string[] mainArgNameList = Enum.GetNames<MainArgs>();
				for (int i = 0; i < mainArgNameList.Length; ++i)
				{
					mainArgTypeDict.Add(mainArgNameList[i], mainArgValueList[i]);
				}
			}

			//	引数の解析
			MainArgsData mainArgsData = new MainArgsData();
			MainArgs targetMainArgType = MainArgs.Invalid;
            for (int i = 0; i < args.Length; ++i)
			{

				string arg = args[i];

				if (arg.Length <= 0 || arg[0] != '-' || mainArgTypeDict.TryGetValue(arg[1..], out MainArgs outMainArgType) == false)
				{

				}
				else
				{
					
					switch(outMainArgType)
					{
						default:
                            targetMainArgType = outMainArgType;

                            continue;
					}
                }

				string replaceArg = arg.Replace("\"", "");

                switch (targetMainArgType)
				{
					case MainArgs.Invalid:
						break;

					case MainArgs.SourceFilePath:
						mainArgsData.SourceFilePath = replaceArg;

                        break;

					case MainArgs.Configuration:
                        mainArgsData.Configuration = replaceArg;

                        break;

					case MainArgs.ConfigurationDefine:
						mainArgsData.BuildSpecDefine = replaceArg;

                        break;

					case MainArgs.SolutionPath:
						mainArgsData.SolutionFilePath = replaceArg;
                        break;

					case MainArgs.OutputDir:
						mainArgsData.OutputDirectory = replaceArg;
                        break;
					
					case MainArgs.Platform:
						mainArgsData.Platform = replaceArg;
                        break;
                    case MainArgs.PlatformDefine:
                        mainArgsData.PlatformDefine = replaceArg;
                        break;

					case MainArgs.ProjectName:
                        break;


					case MainArgs.MSBuildBinPath:
						mainArgsData.MSBuildBinPath = replaceArg;
						break;

					case MainArgs.ProjectPath:
						mainArgsData.ProjectFilePath = replaceArg;
						break;

					case MainArgs.EnableNamespaceList:
                        mainArgsData.EnableNamespaceList = replaceArg.Split(',').ToList();

                        break;

					default:
						System.Diagnostics.Debug.Assert(false);
						break;
                }
            }

            return (int)MainProcess(mainArgsData);
		}

		private static ErrorCode MainProcess(MainArgsData argsData)
		{
            Parser.CppParser parser = new Parser.CppParser();

			if (parser.Parse(
				new Parser.CppParser.SetupParam() 
				{
					SourceFilePath = argsData.SourceFilePath,
					SolutionPath = argsData.SolutionFilePath,
					ProjectFilePath = argsData.ProjectFilePath,
					MSBuildBinPath = argsData.MSBuildBinPath,
					Configuration = argsData.Configuration,
					Platform = argsData.Platform,
					IgnoreNamespaceList = argsData.IgnoreNamespaceList,
					EnableRootNamespaceList = argsData.EnableNamespaceList
                }
				) == false)
			{
				Trace.Error(null, "解析に失敗しました。");
				return ErrorCode.Error;
			}

			if(parser.RootDeclHolder == null)
			{
				return ErrorCode.Error;
			}

			//	ビルドタイムスタンプファイルを解析
			List<string> targetModuleNameList = new List<string>();
            {
				string engineBuildTimeStampFileDirectory = $"{System.IO.Path.GetTempPath()}\nox_build_time_stamp";

                if (System.IO.Directory.Exists(engineBuildTimeStampFileDirectory) == false)
				{
					return ErrorCode.Error;
				}

				string reflectionGenTimeStampDirectory = $"{System.IO.Path.GetTempPath()}\nox_build_time_stamp";

                if (System.IO.Directory.Exists(reflectionGenTimeStampDirectory) == false)
				{
                    System.IO.Directory.CreateDirectory(reflectionGenTimeStampDirectory);
                }

				foreach(string engineTimeStampFileName in System.IO.Directory.GetFiles(engineBuildTimeStampFileDirectory))
				{
					string fileNameWithoutExtension = System.IO.Path.GetFileNameWithoutExtension(engineTimeStampFileName);

                    string reflectionGenTimeStampFilePath = $"{reflectionGenTimeStampDirectory}\\{engineTimeStampFileName}";
					if(System.IO.File.Exists(reflectionGenTimeStampFilePath) == true)
                    {
                        //	タイムスタンプを比較
                        string engineTimeStamp = System.IO.File.ReadAllText(engineTimeStampFileName);
						System.DateTime engineTimeStampDataTime = System.DateTime.Parse(engineTimeStamp);

                        string reflectionGenTimeStamp = System.IO.File.ReadAllText(reflectionGenTimeStampFilePath);
                        System.DateTime reflectionGenTimeStampDataTime = System.DateTime.Parse(reflectionGenTimeStamp);

                        if (engineTimeStampDataTime > reflectionGenTimeStampDataTime)
						{
                            targetModuleNameList.Add(fileNameWithoutExtension);
                        }
                    }
                    else
					{
                        System.IO.File.Create(reflectionGenTimeStampFilePath);
						targetModuleNameList.Add(fileNameWithoutExtension);
                    }
                    
					//	タイムスタンプを更新
                    System.IO.File.WriteAllText(reflectionGenTimeStampFilePath, System.DateTime.Now.ToString());
                }
            }

			//	コード出力
			Generator.Generator generator = new Generator.Generator()
			{
                TargetModuleNameList = targetModuleNameList,
                TypeInfoListWithModuleNameDict = parser.TypeInfoListWithModuleNameDict,
				BuildSpec = argsData.Configuration,
				BuildSpecDefine = argsData.BuildSpecDefine,
				OutputDirectory = argsData.OutputDirectory,
				Platform = argsData.Platform,
				PlatformDefine = argsData.PlatformDefine,

			};

			generator.Generate();

//			generator.Setup(parser, argsData.OutputDirectory, argsData.BuildSpec, argsData.Platform, argsData.BuildSpecDefine, argsData.PlatformDefine);
//			generator.Generate();

            Console.ReadLine();


			return ErrorCode.Success;
		}
		
		#endregion
	}
}