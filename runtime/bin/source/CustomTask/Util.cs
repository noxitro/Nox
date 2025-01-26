using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.CustomTask
{
	public static class Util
	{
		public static Data GetData()
		{
			string path = GetBinFilePath();

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
		/// バイナリファイルのパスを取得する
		/// </summary>
		/// <returns></returns>
		internal static string GetBinFilePath()
		{
			string tempPath = System.IO.Path.GetTempPath();

			string path = System.IO.Path.GetFullPath(tempPath + "/NoxReflectionPreData.bin");

			return path;
		}
	}
}
