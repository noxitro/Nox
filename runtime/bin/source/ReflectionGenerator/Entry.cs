

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

			public string OutputDirectory { get; set; } = string.Empty;
			public string SolutionDirectory { get; set; } = string.Empty;

			public string BuildSpec { get; set; } = string.Empty;
			public string Platform { get; set; } = string.Empty;
			public string BuildSpecDefine { get; set; } = string.Empty;
			public string PlatformDefine { get; set; } = string.Empty;
        }
        #endregion

        #region 非公開メソッド
		private enum MainArgs : byte
		{
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
			BuildSpec,

			/// <summary>
			/// プラットフォーム
			/// </summary>
			Platform,

			/// <summary>
			/// ビルド定義名
			/// </summary>
			BuildSpecDefine,

			/// <summary>
			/// プラットフォーム定義名
			/// </summary>
			PlatformDefine,
			_Max
		}

		class Base
		{
			public Base() { }
			public Base(string name) { }

		}

		private static int Main(string[] args)
		{
            if (args.Length == (int)MainArgs._Max) 
			{
				Trace.Error(null, $"コマンドライン引数が一致しません {args.Length}");
				return -1;
			}

            //  TestMsBuild();

            args = new string[(int)MainArgs._Max]
			{
                "C:\\NitroEngine\\Runtime\\TypeInfo\\CodeGen\\",
                "C:/NitroEngine/Runtime/",
                "Debug",
				"x64",
				"NITRO_DEBUG",
				"NITRO_WIN64"
			};

			MainArgsData mainArgsData = new MainArgsData()
			{
				OutputDirectory = System.IO.Path.GetFullPath(args[(int)MainArgs.OutputDirectory]),
				SolutionDirectory = System.IO.Path.GetFullPath(args[(int)MainArgs.SolutionDirectory]),
				BuildSpec = args[(int)MainArgs.BuildSpec],
				Platform = args[(int)MainArgs.Platform],
				BuildSpecDefine = args[(int)MainArgs.BuildSpecDefine],
				PlatformDefine = args[(int)MainArgs.PlatformDefine],
			};

            return (int)MainProcess(mainArgsData);
		}

		private static ErrorCode MainProcess(MainArgsData argsData)
		{
            Parser.CppParser parser = new Parser.CppParser();

			parser.Parse(
				new Parser.CppParser.SetupParam() 
				{
					SourceFilePath = "D:\\github\\Nox\\runtime\\bin\\source\\ReflectionGenerator\\Sample\\main.cpp",
					IgnoreNamespaceList = new List<string>() { "", "std" }
                }
				);

			//	コード出力
//			Generator.Generator generator = new Generator.Generator();
//			generator.Setup(parser, argsData.OutputDirectory, argsData.BuildSpec, argsData.Platform, argsData.BuildSpecDefine, argsData.PlatformDefine);
//			generator.Generate();

            Console.ReadLine();


			return ErrorCode.Success;
		}
		
		#endregion
	}
}