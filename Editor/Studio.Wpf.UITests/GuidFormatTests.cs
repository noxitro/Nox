// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.Text.RegularExpressions;
using Xunit;

namespace Studio.Wpf.UITests;

/// <summary>
/// System.Guid のバイト列と文字列の形が、Runtime の nox::Guid と一致することを確かめる。
/// </summary>
/// <remarks>
/// 値は runtime/kernel/test/guid_vectors.inc にあり、Runtime の kernel_test (guid_test.cpp) も同じファイルを読む。
/// 保存データと通信には "D" の形を書く。読むときは "D" と "N" を受け付け、大文字と小文字を区別しない。
/// UI を動かさないテストだが、Editor の C# のテストのプロジェクトがここだけなので置いている。
/// </remarks>
public sealed partial class GuidFormatTests
{
	private const string VectorsRelativePath = "runtime/kernel/test/guid_vectors.inc";

	[GeneratedRegex("""^NOX_GUID_VECTOR\("([0-9a-f]{32})",\s*"([^"]*)",\s*"([^"]*)"\)""", RegexOptions.Multiline)]
	private static partial Regex VectorPattern();

	[GeneratedRegex("""^NOX_GUID_INVALID\("([^"]*)"\)""", RegexOptions.Multiline)]
	private static partial Regex InvalidPattern();

	public static TheoryData<string, string, string> Vectors()
	{
		TheoryData<string, string, string> data = new();
		foreach (Match match in VectorPattern().Matches(ReadVectorsFile()))
		{
			data.Add(match.Groups[1].Value, match.Groups[2].Value, match.Groups[3].Value);
		}
		return data;
	}

	public static TheoryData<string> InvalidTexts()
	{
		TheoryData<string> data = new();
		foreach (Match match in InvalidPattern().Matches(ReadVectorsFile()))
		{
			data.Add(match.Groups[1].Value);
		}
		return data;
	}

	[Fact]
	public void VectorsAreLoaded()
	{
		// 正規表現の書き間違いで 0 件になっても Theory は素通りしてしまうので件数を確かめる
		Assert.True(VectorPattern().Matches(ReadVectorsFile()).Count >= 3);
		Assert.True(InvalidPattern().Matches(ReadVectorsFile()).Count >= 5);
	}

	[Theory]
	[MemberData(nameof(Vectors))]
	public void FormatsMatchRuntimeNoxGuid(string bytesHex, string hyphenated, string compact)
	{
		byte[] bytes = Convert.FromHexString(bytesHex);
		var guid = new Guid(bytes);
		Assert.Equal(hyphenated, guid.ToString("D"));
		Assert.Equal(hyphenated, guid.ToString());
		Assert.Equal(compact, guid.ToString("N"));
		Assert.Equal(bytes, guid.ToByteArray());
	}

	[Theory]
	[MemberData(nameof(Vectors))]
	public void ParseAcceptsHyphenatedAndCompactInAnyCase(string bytesHex, string hyphenated, string compact)
	{
		byte[] bytes = Convert.FromHexString(bytesHex);
		foreach (string text in new[] { hyphenated, compact, hyphenated.ToUpperInvariant(), compact.ToUpperInvariant() })
		{
			Assert.True(TryParseLikeRuntime(text, out Guid parsed), text);
			Assert.Equal(bytes, parsed.ToByteArray());
		}
	}

	[Theory]
	[MemberData(nameof(InvalidTexts))]
	public void ParseRejectsOtherForms(string text)
	{
		Assert.False(TryParseLikeRuntime(text, out _), $"[{text}]");
	}

	/// <summary>Runtime の nox::Guid::TryParse と同じものを受け付ける読み方。</summary>
	private static bool TryParseLikeRuntime(string text, out Guid guid)
	{
		return Guid.TryParseExact(text, "D", out guid) || Guid.TryParseExact(text, "N", out guid);
	}

	private static string ReadVectorsFile()
	{
		for (DirectoryInfo? directory = new(AppContext.BaseDirectory); directory != null; directory = directory.Parent)
		{
			string path = Path.Combine(directory.FullName, VectorsRelativePath);
			if (File.Exists(path))
			{
				return File.ReadAllText(path);
			}
		}

		throw new FileNotFoundException($"{VectorsRelativePath} が、テストの出力先から上のどのフォルダにも見つかりません。");
	}
}
