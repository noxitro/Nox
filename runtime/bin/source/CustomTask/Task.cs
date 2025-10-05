using System;
namespace Nox.CustomTask
{
	//internal static class Entry
	//{
	//	private static int Main()
	//	{
	//		Data data = new Data();

	//	//	string path = "D:\\github\\Nox\\runtime\\bin\\source\\test\\data.bin";

	//		const string path = "D:\\github\\Nox\\runtime\\bin\\source\\test\\test.bin";
	//	//	const string path2 = "D:\\github\\Nox\\runtime\\bin\\source\\test\\test.txt";
	//		//using (System.IO.StreamWriter streamWriter = new System.IO.StreamWriter(path, false, System.Text.Encoding.UTF8))
	//		{
	//		}

	//		using (System.IO.FileStream fileStream = new System.IO.FileStream(path, System.IO.FileMode.Create, System.IO.FileAccess.Write))
	//		{
	//			System.Runtime.Serialization.Formatters.Binary.BinaryFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
	//			formatter.Serialize(fileStream, data);
	//		}

	//		return 10;
	//	}
	//}

	/// <summary>
	/// リフレクション生成に必要な情報を作成する
	/// </summary>
	public class Task : Microsoft.Build.Utilities.Task
	{
		#region 公開プロパティ
		// ソースコードファイル名
		[Microsoft.Build.Framework.Required]
		public string SourceFiles { get; set; } = string.Empty;

		/// <summary>
		/// 追加インクルードディレクトリ
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string AdditionalIncludeDirectories { get; set; } = string.Empty;

		/// <summary>
		/// c++バージョン
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string CppVersion { get; set; } = string.Empty;

		/// <summary>
		/// プリプロセッサマクロ定義
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string PreprocessorMacro { get; set; } = string.Empty;

		/// <summary>
		/// 追加オプション
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string AdditionalOptions { get; set; } = string.Empty;

		/// <summary>
		/// 解析対象のソースファイル
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string ReflectionTargetSourceFile { get; set; } = string.Empty;

		/// <summary>
		/// ソリューションパス
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string SolutionPath { get; set; } = string.Empty;

		/// <summary>
		/// ソリューションディレクトリ
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string SolutionDir { get; set; } = string.Empty;

		/// <summary>
		/// reflection_generatedのプロジェクトパス
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string ProjectPath { get; set; } = string.Empty;

		/// <summary>
		/// reflection_generatedのプロジェクトディレクトリ
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string ProjectDir { get; set; } = string.Empty;

		/// <summary>
		/// コード出力先ディレクトリ
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string OutputGenerateDir { get; set; } = string.Empty;

		/// <summary>
		/// 構成
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string Configuration { get; set; } = string.Empty;

		/// <summary>
		/// 構成定義
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string ConfigurationDefine { get; set; } = string.Empty;

		/// <summary>
		/// プラットフォーム
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string Platform { get; set; } = string.Empty;

		/// <summary>
		/// プラットフォーム定義
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string PlatformDefine { get; set; } = string.Empty;

		/// <summary>
		/// 出力ディレクトリ
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string OutDir { get; set; } = string.Empty;

		/// <summary>
		/// 中間ディレクトリ
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string IntermediateOutputPath { get; set; } = string.Empty;

		/// <summary>
		/// MSBuild のバイナリパス
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string MSBuildBinPath { get; set; } = string.Empty;

		/// <summary>
		/// 最適化オプション
		/// </summary>
		[Microsoft.Build.Framework.Required]
		public string Optimization { get; set; } = string.Empty;

		public string BuildLogFile { get; set;} = string.Empty;

		/// <summary>
		/// RTTIを使用するか
		/// </summary>
		public bool UseRtti { get; set; } = false;
		#endregion

		#region 公開メソッド
		public override bool Execute()
		{
			Log.LogMessage(Microsoft.Build.Framework.MessageImportance.High, "This is a log message from MyCustomTask.");
			
			if (Generate() == false)
			{
				return false;
			}

			return true;
		}
		#endregion

		#region 非公開メソッド
		private bool Generate()
		{
			Data data = new Data()
			{
				CppVersion = CppVersion,
				PreprocessorMacro = PreprocessorMacro,
				AdditionalOptions = AdditionalOptions,
				ReflectionTargetSourceFile = ReflectionTargetSourceFile,
				SolutionPath = SolutionPath,
				SolutionDir = SolutionDir,
				ProjectPath = ProjectPath,
				ProjectDir = ProjectDir,
				OutputGenerateDir = OutputGenerateDir,
				Configuration = Configuration,
				ConfigurationDefine = ConfigurationDefine,
				Platform = Platform,
				PlatformDefine = PlatformDefine,
				OutDir = OutDir,
				IntermediateOutputPath = IntermediateOutputPath,
				MSBuildBinPath = MSBuildBinPath,
				Optimization = Optimization,
				AdditionalIncludeDirectories = AdditionalIncludeDirectories,
				UseRtti = UseRtti,
			};

			string path = Util.GetBinFilePath();
			WriteToFile(path, data);

			return true;
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
		#endregion
	}
}
