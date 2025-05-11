using System;
namespace Nox.CustomTask
{
	file static class Entry
	{
		private static int Main()
		{
			Data data = new Data
			{
				SolutionPath = "SolutionPath",
				Configuration = "Configuration",
				PreprocessorMacro = "PreprocessorMacro",
				AdditionalOptions = "AdditionalOptions",
			};

			string jsonStr = System.Text.Json.JsonSerializer.Serialize(data);
			return 20;
		}
	}

	/// <summary>
	/// リフレクション生成に必要な情報を作成する
	/// </summary>
	public class Task : Microsoft.Build.Utilities.Task
	{
		#region 公開プロパティ
		// ソースコードファイル名
		public string SourceFiles { get; set; } = string.Empty;
		// インクルードパス
		public string IncludePaths { get; set; } = string.Empty;

		/// <summary>
		/// プリプロセッサマクロ定義
		/// </summary>
		public string PreprocessorMacro { get; set; } = string.Empty;

		/// <summary>
		/// 追加オプション
		/// </summary>
		public string AdditionalOptions { get; set; } = string.Empty;

		/// <summary>
		/// 解析対象のソースファイル
		/// </summary>
		public string ReflectionTargetSourceFile { get; set; } = string.Empty;

		/// <summary>
		/// ソリューションパス
		/// </summary>
		public string SolutionPath { get; set; } = string.Empty;

		/// <summary>
		/// 構成
		/// </summary>
		public string Configuration { get; set; } = string.Empty;

		/// <summary>
		/// 構成定義
		/// </summary>
		public string ConfigurationDefine { get; set; } = string.Empty;

		/// <summary>
		/// プラットフォーム
		/// </summary>
		public string Platform { get; set; } = string.Empty;

		/// <summary>
		/// プラットフォーム定義
		/// </summary>
		public string PlatformDefine { get; set; } = string.Empty;

		/// <summary>
		/// 中間ディレクトリ
		/// </summary>
		public string IntermediateOutputPath { get; set; } = string.Empty;

		/// <summary>
		/// MSBuild のバイナリパス
		/// </summary>
		public string MSBuildBinPath { get; set; } = string.Empty;

		/// <summary>
		/// 最適化オプション
		/// </summary>
		public string Optimization { get; set; } = string.Empty;

		public string BuildLogFile { get; set; } = string.Empty;
		#endregion

		#region 公開メソッド
		public override bool Execute()
		{
			Log.LogMessage(Microsoft.Build.Framework.MessageImportance.High, "This is a log message from MyCustomTask.");

			// 解析処理
			// 成功したらtrue を返す
			const string path = "D:\\github\\Nox\\runtime\\bin\\source\\test\\test.txt";
			using (System.IO.StreamWriter streamWriter = new System.IO.StreamWriter(path, false, System.Text.Encoding.UTF8))
			{
				streamWriter.WriteLine($"SourceFiles:\n{SourceFiles}");
				streamWriter.WriteLine("");
				streamWriter.WriteLine("");

				streamWriter.WriteLine($"IncludePaths:\n{IncludePaths}");

				streamWriter.WriteLine("");
				streamWriter.WriteLine("");

				streamWriter.WriteLine($"PreprocessorMacro:\n{PreprocessorMacro}");
				streamWriter.WriteLine("");
				streamWriter.WriteLine("");

				streamWriter.WriteLine($"{nameof(AdditionalOptions)}:\n{AdditionalOptions}");
				streamWriter.WriteLine("");
				streamWriter.WriteLine("");

				streamWriter.WriteLine($"{nameof(BuildLogFile)}:\n{BuildLogFile}");
				streamWriter.WriteLine("");
				streamWriter.WriteLine("");

			}

			GenerateJson();

			return true;
		}
		#endregion

		#region 非公開メソッド
		private bool GenerateJson()
		{

			Data data = new Data
			{
				SolutionPath = SolutionPath,
				Configuration = Configuration,
				PreprocessorMacro = PreprocessorMacro,
				AdditionalOptions = AdditionalOptions,
			};

			//	string jsonStr = System.Text.Json.JsonSerializer.Serialize(data);

			////	中間ディレクトリへ書き込む
			//string path = "D:\\github\\Nox\\runtime\\bin\\source\\test\\data.json";
			//if (System.IO.File.Exists(path) == false)
			//{
			//	System.IO.File.Create(path);
			//}
			//System.IO.File.WriteAllText("D:\\github\\Nox\\runtime\\bin\\source\\test\\data.json", jsonStr, System.Text.Encoding.UTF8);

			return true;
		}
		#endregion
	}
}
