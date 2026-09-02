using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.CustomTask;

public static class Util
{
	public static Data GetData(string outputGenerateDir, string platform, string configuration)
	{
		string path = GetBinFilePath(outputGenerateDir, platform, configuration);

		byte[] buffer = System.IO.File.ReadAllBytes(path);
		Data example = new Data();

		unsafe
		{
			fixed (byte* p = buffer)
			{
				System.Runtime.InteropServices.Marshal.PtrToStructure((IntPtr)p, example);
			}
		}

		return example;
	}

	/// <summary>
	/// 前処理データ(バイナリ)のフルパスを取得する
	/// </summary>
	/// <remarks>
	/// %TEMP% のようなマシン共通の場所には置かない。同じリポジトリの複数ワークツリーを
	/// 同時にビルドすると互いのファイルを上書きしてしまうため、ソースツリーごとに異なる
	/// 生成出力ディレクトリ($(ProjectDir)gen)配下へ置き、さらにプラットフォーム/構成を
	/// ファイル名に含める。書き手(CustomTask)と読み手(ReflectionGenerator.exe)は
	/// 必ずこのメソッドを経由してパスを決めること。
	/// </remarks>
	/// <param name="outputGenerateDir">生成出力ディレクトリ (MSBuild の OutputGenerateDir / exe の -out)</param>
	/// <param name="platform">プラットフォーム名 (x64 など)</param>
	/// <param name="configuration">構成名 (Debug など)</param>
	public static string GetBinFilePath(string outputGenerateDir, string platform, string configuration)
	{
		if (string.IsNullOrEmpty(outputGenerateDir))
		{
			throw new ArgumentException("outputGenerateDir が空です。", nameof(outputGenerateDir));
		}

		string fileName = FILE_BASE_NAME + "." + platform + "." + configuration + FILE_EXTENSION;

		return System.IO.Path.GetFullPath(System.IO.Path.Combine(outputGenerateDir, fileName));
	}

	private const string FILE_BASE_NAME = "NoxReflectionPreData";
	private const string FILE_EXTENSION = ".bin";
}
