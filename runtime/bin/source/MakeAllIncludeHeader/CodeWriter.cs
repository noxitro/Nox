// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MakeAllIncludeHeader;

	public class CodeWriter : IDisposable
	{
		public enum ScopeType
		{
			/// <summary>
			/// 宣言 {};
			/// </summary>
			Decl,

			/// <summary>
			/// 定義 {}
			/// </summary>
			Define,

			/// <summary>
			/// 宣言(丸括弧) ();
			/// </summary>
			Decl_Paren
		}

		#region 非公開フィールド
		private StreamWriter _Stream;

		private Stack<ScopeType> _ScopeStack = new Stack<ScopeType>();
		private Stack<string> _NestList = new Stack<string>();

		/// <summary>
		/// 書き出し先のパス。閉じるまではメモリ (_Stream の下の MemoryStream) に溜める
		/// </summary>
		private readonly string _FilePath;

		private bool _IsClosed = false;
		#endregion

		/// <summary>
		/// 閉じたときに path へ書き出します。中身が既存のファイルと同じなら書き込みません
		/// </summary>
		/// <param name="path"></param>
		public CodeWriter(string path)
		{
			//	BOM なしの UTF-8 で書く (Encoding.UTF8 は BOM を付ける)。AGENTS.md の「ファイル形式」
			_Stream = new StreamWriter(new MemoryStream(), new UTF8Encoding(false));
			_FilePath = path;
		}


		public void Dispose()
		{
			//  ネストのエラーチェック
			if (_NestList.Count > 0 || _ScopeStack.Count > 0)
			{
#if DEBUG
				Console.WriteLine("ネストが正しく記述されていません thisName: ", _FilePath);
#endif
			}

			Close();
		}

		/// <summary>
		/// 溜めた中身をファイルへ書き出します。
		/// 既存のファイルと中身が同じなら書き込まず、更新日時を変えない。all_include.h は app.h から
		/// 読まれるので、更新日時が毎回新しくなると、中身が同じでも app と runtime が再コンパイルされる
		/// </summary>
		public void Close()
		{
			if (_IsClosed)
			{
				return;
			}
			_IsClosed = true;

			_Stream.Flush();
			MemoryStream memoryStream = (MemoryStream)_Stream.BaseStream;
			ReadOnlySpan<byte> content = memoryStream.GetBuffer().AsSpan(0, (int)memoryStream.Length);
			if (IsSameContent(_FilePath, content) == false)
			{
				using FileStream fileStream = new(_FilePath, FileMode.Create, FileAccess.Write);
				fileStream.Write(content);
			}
			_Stream.Close();
		}

		private static bool IsSameContent(string path, ReadOnlySpan<byte> content)
		{
			FileInfo fileInfo = new(path);
			if (fileInfo.Exists == false || fileInfo.Length != content.Length)
			{
				return false;
			}
			return File.ReadAllBytes(path).AsSpan().SequenceEqual(content);
		}



		public void Push()
		{
			_NestList.Push("\t");
		}

		public void PushScope(ScopeType scopeType)
		{
			_ScopeStack.Push(scopeType);

			switch (scopeType)
			{
				case ScopeType.Define:
				case ScopeType.Decl:
					WriteLine("{");
					break;

				case ScopeType.Decl_Paren:
					WriteLine("(");
					break;

				default:
					System.Diagnostics.Debug.Assert(false, $"未実装の項目:{scopeType.ToString()}");
					break;
			}

			Push();
		}

		public void Pop()
		{
			_NestList.Pop();
		}

		public void PopScope()
		{
			Pop();  //  先にネストを抜けてから

			ScopeType scopeType = _ScopeStack.Pop();

			switch (scopeType)
			{
				case ScopeType.Decl:
					WriteLine("};");
					break;

				case ScopeType.Define:
					WriteLine("}");
					break;

				case ScopeType.Decl_Paren:
					WriteLine(");");
					break;

				default:
					System.Diagnostics.Debug.Assert(false, $"未実装の項目:{scopeType.ToString()}");
					break;
			}

		}

		/// <summary>
		/// スコープ無しの書き込み
		/// </summary>
		/// <param name="str"></param>
		public void WriteLineNoneScope(string str)
		{
			_Stream.WriteLine(str);
		}

		public void WriteLineNoneScope(string str, params object[] args) => WriteLineNoneScope(string.Format(str, args));

		public void Write(string str)
		{

			_Stream.Write(str);
		}

		public void WriteNoneScope(string str)
		{
			_Stream.WriteLine(str);
		}

		public void WriteNest()
		{
			string str = string.Empty;
			foreach (var nest in _NestList)
			{
				str = nest + str;
			}
			_Stream.Write(str);
		}

		/// <summary>
		/// スコープありの書き込み
		/// </summary>
		/// <param name="str"></param>
		public void WriteLine(string str)
		{
			foreach (var nest in _NestList)
			{
				str = nest + str;
			}
			_Stream.WriteLine(str);
		}

		public void WriteLine(string str, params object[] args)
		{
			WriteLine(string.Format(str, args));
		}

		public void WriteNewLine(int line = 1)
		{
			for (int i = 0; i < line; i++)
			{
				_Stream.WriteLine(string.Empty);
			}
		}


	}
