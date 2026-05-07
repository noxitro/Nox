using System;
using System.Collections.Generic;
using System.Text;

namespace Nox
{
	public static class CodeWriterExtensions
	{
		//  タイトル
		public static void WriteLineSource(this BaseCodeWriter codeWriter, ReadOnlySpan<char> from)
		{
			WriteLineCopyRight(codeWriter);
			codeWriter.WriteLine("//\tdo not edit");
			codeWriter.WriteLine($"//\twritten from {from}");
		}

		/// <summary>
		/// ヘッダ用のタイトル
		/// </summary>
		public static void WriteLineHeader(this BaseCodeWriter codeWriter, ReadOnlySpan<char> from)
		{
			codeWriter.WriteLine("#pragma once");
			codeWriter.WriteLine("//\tdo not edit");
			codeWriter.WriteLine($"//\twritten from {from}");
		}

		public static void WriteLinePPIf(this BaseCodeWriter codeWriter, ReadOnlySpan<char> s)
		{
			codeWriter.WriteLine($"#if\t{s}");
		}

		public static void WriteLinePPIfNot(this BaseCodeWriter codeWriter, ReadOnlySpan<char> s)
		{
			codeWriter.WriteLine($"#if\t!{s}");
		}

		public static void WriteLinePPIfDefine(this BaseCodeWriter codeWriter, ReadOnlySpan<char> s)
		{
			codeWriter.WriteLine($"#if\tdefined({s})");
		}

		public static void WriteLinePPIfNotDefine(this BaseCodeWriter codeWriter, ReadOnlySpan<char> s)
		{
			codeWriter.WriteLine($"#if\t!defined({s})");
		}

		public static void WriteLinePPEndIf(this BaseCodeWriter codeWriter, ReadOnlySpan<char> s)
		{
			codeWriter.WriteLine($"#endif\t//\t{s}");
		}

		public static void WriteLinePragmaOnce(this BaseCodeWriter codeWriter)
		{
			codeWriter.WriteLine("#pragma once");
		}

		public static void WriteLineCopyRight(this BaseCodeWriter codeWriter)
		{
			codeWriter.WriteLine($"//\tCopyright (c) {DateTime.Now.Year.ToString()} NOX ENGINE All rights reserved.");
		}

		public static void WriteIncludePch(this BaseCodeWriter codeWriter)
		{
			codeWriter.WriteLine("#include\t\"pch.h\"");
		}

		public static void WriteLineInclude(this BaseCodeWriter codeWriter, ReadOnlySpan<char> includePath)
		{
			codeWriter.WriteLine($"#include\t\"{includePath}\"");
		}

		public static void WriteNamespace(this BaseCodeWriter codeWriter, ReadOnlySpan<char> namespaceName)
		{
			codeWriter.WriteLine($"namespace {namespaceName}");
		}

		public static void WriteLineRegion(this BaseCodeWriter codeWriter, ReadOnlySpan<char> name)
		{
			codeWriter.WriteLineIgnoreNest($"#pragma region {name}");
		}
		public static void WriteLineEndRegion(this BaseCodeWriter codeWriter, ReadOnlySpan<char> comment)
		{
			codeWriter.WriteLineIgnoreNest($"#pragma endregion\t//\t{comment}");
		}
	}
}
