
using System.Runtime.InteropServices;

namespace Nox.CustomTask
{
	//[System.Serializable]
	[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential, Pack = 1)]
	public class Data
	{
		const int StringSize = 256;

		/// <summary>
		/// c++バージョン
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string CppVersion;

		/// <summary>
		/// プリプロセッサマクロ定義
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string PreprocessorMacro;

		/// <summary>
		/// 追加オプション
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string AdditionalOptions;

		/// <summary>
		/// 解析対象のソースファイル
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string ReflectionTargetSourceFile;

		/// <summary>
		/// ソリューションパス
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string SolutionPath;

		/// <summary>
		/// ソリューションディレクトリ
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string SolutionDir;

		/// <summary>
		/// reflection_generatedのプロジェクトパス
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string ProjectPath;

		/// <summary>
		/// reflection_generatedのプロジェクトディレクトリ
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string ProjectDir;

		/// <summary>
		/// コード出力先ディレクトリ
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string OutputGenerateDir;

		/// <summary>
		/// 構成
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string Configuration;

		/// <summary>
		/// 構成定義
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string ConfigurationDefine;

		/// <summary>
		/// プラットフォーム
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string Platform;

		/// <summary>
		/// プラットフォーム定義
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string PlatformDefine;

		/// <summary>
		/// 出力ディレクトリ
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string OutDir;

		/// <summary>
		/// 中間ディレクトリ
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string IntermediateOutputPath;

		/// <summary>
		/// MSBuild のバイナリパス
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string MSBuildBinPath;

		/// <summary>
		/// 最適化オプション
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string Optimization;

		/// <summary>
		/// 追加のインクルードディレクトリ
		/// </summary>
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = StringSize)]
		public string AdditionalIncludeDirectories;

		/// <summary>
		/// 実行時型情報を利用するか
		/// </summary>
		public bool UseRtti;
	}
}
