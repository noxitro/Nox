using System;
using System.IO;

namespace Core;

	/// <summary>
	/// ネイティブアセットファイルのオンディスクフォーマット定義（C++ runtime/core/asset_format.h のミラー）。
	/// レイアウト: [FileHeader][ChunkHeader...][RawData]（リトルエンディアン・パディングなし）。
	/// </summary>
	public static class AssetNativeFormat
	{
		/// <summary>マジック 'N''O''X''A' をリトルエンディアン u32 で表現。</summary>
		public const uint Magic = 0x41584F4E;

		/// <summary>フォーマットバージョン。</summary>
		public const ushort FormatVersion = 1;

		/// <summary>
		/// 出力先プラットフォームフォルダ名。
		/// C++ の GetPlatformTypeName の優先順（Win64 &gt; Android &gt; Studio）と一致させること。
		/// </summary>
		public const string PlatformName = "Win64";

		/// <summary>FileHeader のオンディスクサイズ（u32 + u16 + u16）。</summary>
		public const int FileHeaderSize = sizeof(uint) + sizeof(ushort) + sizeof(ushort);

		/// <summary>ChunkHeader のオンディスクサイズ（u8 + u32 * 3）。</summary>
		public const int ChunkHeaderSize = sizeof(byte) + sizeof(uint) * 3;

		public enum ChunkType : byte
		{
			Invalid = 0,
			Main = 1,
			Extra = 2,
		}
	}

	/// <summary>
	/// ネイティブアセットファイルのバイト列を構築する。
	/// </summary>
	public static class NativeAssetWriter
	{
		/// <summary>
		/// Main チャンク1個だけを持つネイティブファイルのバイト列を構築する。
		/// </summary>
		public static byte[] BuildSingleMainChunk(byte[] mainData, uint chunkVersion = 0)
		{
			ArgumentNullException.ThrowIfNull(mainData);

			const int chunkCount = 1;
			int dataOffset = AssetNativeFormat.FileHeaderSize + AssetNativeFormat.ChunkHeaderSize * chunkCount;

			using MemoryStream stream = new(dataOffset + mainData.Length);
			using BinaryWriter writer = new(stream);

			// FileHeader
			writer.Write(AssetNativeFormat.Magic);
			writer.Write(AssetNativeFormat.FormatVersion);
			writer.Write((ushort)chunkCount);

			// Main ChunkHeader
			writer.Write((byte)AssetNativeFormat.ChunkType.Main);
			writer.Write((uint)dataOffset);
			writer.Write((uint)mainData.Length);
			writer.Write(chunkVersion);

			// RawData
			writer.Write(mainData);

			writer.Flush();
			return stream.ToArray();
		}
	}

	/// <summary>
	/// ソースアセットからネイティブファイルへ変換する。
	/// 現状は Main チャンク = ソース生バイトのパススルー（importer 種別ごとの変換は将来拡張）。
	/// </summary>
	public static class NativeAssetConverter
	{
		/// <summary>
		/// 指定 URI のアセットをネイティブファイルへ変換して書き出す。
		/// </summary>
		/// <param name="workspace">対象ワークスペース</param>
		/// <param name="assetUri">アセット URI（例: "assets:/foo/bar.scene"）</param>
		/// <returns>変換に成功したか</returns>
		public static bool Convert(Workspace workspace, string assetUri)
		{
			ArgumentNullException.ThrowIfNull(workspace);

			string relativePath = ExtractRelativePath(assetUri);
			if (string.IsNullOrEmpty(relativePath))
			{
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"Invalid asset uri for convert: {assetUri}");
				return false;
			}

			string nativeRelative = relativePath.Replace('/', Path.DirectorySeparatorChar);
			string sourcePath = Path.Combine(workspace.AssetRootPath, nativeRelative);
			if (File.Exists(sourcePath) == false)
			{
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"Convert source not found: {sourcePath}");
				return false;
			}

			byte[] sourceBytes = File.ReadAllBytes(sourcePath);
			byte[] nativeBytes = NativeAssetWriter.BuildSingleMainChunk(sourceBytes);

			string destPath = Path.Combine(workspace.ProjectPath, "native", AssetNativeFormat.PlatformName, nativeRelative);
			Directory.CreateDirectory(Path.GetDirectoryName(destPath) ?? string.Empty);
			File.WriteAllBytes(destPath, nativeBytes);
			return true;
		}

		/// <summary>
		/// URI からアセットルート相対パスを取り出す（C++ GetNativeResourcePath と同じ規則）。
		/// </summary>
		private static string ExtractRelativePath(string assetUri)
		{
			if (string.IsNullOrWhiteSpace(assetUri))
			{
				return string.Empty;
			}

			string path = assetUri;
			if (path.StartsWith("assets:/", StringComparison.OrdinalIgnoreCase))
			{
				path = path["assets:/".Length..];
			}
			else if (path.StartsWith("assets/", StringComparison.OrdinalIgnoreCase))
			{
				path = path["assets/".Length..];
			}

			return path.TrimStart('/', '\\');
		}
	}
