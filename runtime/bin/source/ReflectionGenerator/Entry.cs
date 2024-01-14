

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
			public string SolutionDirectory { get; set; } = string.Empty;

			public string Configuration { get; set; } = string.Empty;
			public string Platform { get; set; } = string.Empty;
			public string BuildSpecDefine { get; set; } = string.Empty;
			public string PlatformDefine { get; set; } = string.Empty;

			public string MSBuildBinPath { get; set; } = string.Empty;
			public string ProjectFilePath { get; set; } = string.Empty;

			public List<string> IgnoreNamespaceList { get; } = new List<string>();

			public List<string> EnableNamespaceList { get; } = new List<string>();
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
			OutputDirectory,

			/// <summary>
			/// ソリューションディレクトリ
			/// </summary>
			SolutionDirectory,

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

			_Max
		}

		class Base
		{
			public Base() { }
			public Base(string name) { }

		}

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
                reflectionGenerateArgsFilePath = $"D:\\github\\Nox\\runtime\\reflectionGenerateArgs.txt";
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
                        mainArgsData.Configuration = arg;

                        break;

					case MainArgs.ConfigurationDefine:
						mainArgsData.BuildSpecDefine = arg;

                        break;

					case MainArgs.SolutionDirectory:
						mainArgsData.SolutionDirectory = arg;
                        break;

					case MainArgs.OutputDirectory:
						mainArgsData.OutputDirectory = arg;
                        break;
					
					case MainArgs.Platform:
						mainArgsData.Platform = arg;
                        break;
                    case MainArgs.PlatformDefine:
                        mainArgsData.PlatformDefine = arg;
                        break;

					case MainArgs.ProjectName:

                        break;


					case MainArgs.MSBuildBinPath:
						mainArgsData.MSBuildBinPath = replaceArg;
						break;

					case MainArgs.ProjectPath:
						mainArgsData.ProjectFilePath = arg;
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

			parser.Parse(
				new Parser.CppParser.SetupParam() 
				{
					SourceFilePath = argsData.SourceFilePath,
					ProjectFilePath = argsData.ProjectFilePath,
					MSBuildBinPath = argsData.MSBuildBinPath,
					Configuration = argsData.Configuration,
					Platform = argsData.Platform,
					IgnoreNamespaceList = argsData.IgnoreNamespaceList,
					EnableRootNamespaceList = argsData.EnableNamespaceList
                }
				);

			//	コード出力
			Generator.Generator generator = new Generator.Generator()
			{
				Parser = parser,
				BuildSpec = argsData.Configuration,
				BuildSpecDefine = argsData.BuildSpecDefine,
				OutputDirectory = argsData.OutputDirectory,
				Platform = argsData.Platform,
				PlatformDefine = argsData.PlatformDefine,

			};
//			generator.Setup(parser, argsData.OutputDirectory, argsData.BuildSpec, argsData.Platform, argsData.BuildSpecDefine, argsData.PlatformDefine);
//			generator.Generate();

            Console.ReadLine();


			return ErrorCode.Success;
		}
		
		#endregion
	}
}