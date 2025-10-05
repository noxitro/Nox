using System;
using System.Collections.Generic;
using System.Linq;

namespace ReflectionGenerator
{
    public abstract class BaseCodeWriter
    {
		/// <summary>
		/// ネストの深さ
		/// </summary>
		protected uint _NestDepth = 0;

        public readonly struct IndentScope : IDisposable
        {
            private readonly BaseCodeWriter _Writer;
            public IndentScope(BaseCodeWriter w)
            {
                _Writer = w;
                _Writer.Push();
            }
            void IDisposable.Dispose()
            {
                _Writer.Pop();
            }
        }

        public void Push()
		{
			++_NestDepth;
        }

		public void Pop()
		{
			--_NestDepth;
        }

        public IndentScope Indent() => new IndentScope(this);

        public abstract void Write(ReadOnlySpan<char> str);
        public abstract void Write<T>(string str, params T[] args) where T : struct;
        public abstract void WriteLine(ReadOnlySpan<char> str);
        public abstract void WriteLine<T>(string str, T args0, params T[] args) where T : struct;
        public abstract void WriteLine(string str, string args0, params string[] args);
		public void WriteNewLine(uint line = 1)
		{
			Span<char> newLine = stackalloc char[(int)line*2];
			for (int i = 0; i < line; i++)
			{
				newLine[i] = '\r'; //  改行文字を追加
				newLine[i+1] = '\n'; //  改行文字を追加
			}
			Write(newLine);
		}

		public void WriteNest()
		{
            if (_NestDepth == 0) return; //  ネストがない場合は何もしない
			Span<char> nestStr = stackalloc char[(int)_NestDepth];
			for (int i = 0; i < _NestDepth; i++)
			{
				nestStr[i] = '\t'; //  タブ文字を追加
			}
			Write(nestStr);
		}

        public void WriteLineIgnoreNest(ReadOnlySpan<char> str)
        {
            Write(str);
            WriteNewLine(1);
		}
	}

    /// <summary>
    /// コードジェネレータ
    /// </summary>
    public class CodeWriter : BaseCodeWriter, IDisposable
	{
        #region 型定義
        public enum ScopeType : byte
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
        #endregion

        #region 非公開フィールド
		private readonly StreamWriter _Stream;

        private Stack<ScopeType> _ScopeStack = new Stack<ScopeType>();

#if DEBUG
		private readonly string _FilePath;
#endif
#endregion

        #region 公開メソッド
        /// <summary>
        /// ファイルが存在しない場合作成します
        /// </summary>
        /// <param name="path"></param>
        public CodeWriter(string path)
        {
            _Stream = new System.IO.StreamWriter(path, false, System.Text.Encoding.UTF8);

            //  ファイルをクリア
            _Stream.BaseStream.SetLength(0);

#if DEBUG
            _FilePath = path;
#endif
        }

        public void Dispose()
        {
            //  ネストのエラーチェック
            if (_NestDepth > 0 || _ScopeStack.Count > 0)
            {
#if DEBUG
                Trace.ErrorLine("ネストが正しく記述されていません thisName: ", _FilePath);
#endif
            }

            _Stream.Close();
        }

        public void Close() => _Stream.Close();

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

        public override void Write(ReadOnlySpan<char> str)
        {
            _Stream.Write(str);
        }

		public override void Write<T>(string str, params T[] args) where T : struct
		{
			_Stream.WriteLine(str, args);
		}

		public override void WriteLine(ReadOnlySpan<char> str)
        {
            WriteNest();
			_Stream.WriteLine(str);
		}

        public override void WriteLine<T>(string str, T args0, params T[] args) where T : struct
		{
			WriteNest();
			_Stream.WriteLine(str, args0, args);
		}
		public override void WriteLine(string str, string args0, params string[] args)
		{
			WriteNest();
			_Stream.WriteLine(str, args0, args);
		}

        #endregion

        #region 非公開メソッド
        #endregion
	}

    public class CodeStringBuilder : BaseCodeWriter
    {
        #region フィールド
        private readonly System.Text.StringBuilder _StringBuilder = new System.Text.StringBuilder();
        #endregion

        #region 公開メソッド
        public void Clear()
        {
            _StringBuilder.Clear();
		}

        public override void Write(ReadOnlySpan<char> str)
        {
			_StringBuilder.Append(str);
		}

        public override void Write<T>(string str, params T[] args) where T : struct
        {
            _StringBuilder.AppendFormat(str, args);
		}

        public override void WriteLine(ReadOnlySpan<char> str)
        {
            WriteNest();
			_StringBuilder.Append(str);
            _StringBuilder.AppendLine();
        }
        public override void WriteLine<T>(string str, T args0, params T[] args) where T : struct
        {
            WriteNest();
			_StringBuilder.AppendFormat(str, args0, args);
            _StringBuilder.AppendLine();
        }

        public override void WriteLine(string str, string args0, params string[] args)
        {
            WriteNest();
            _StringBuilder.AppendFormat(str, args0, args);
            _StringBuilder.AppendLine();
        }

        public override string ToString()
        {
            return _StringBuilder.ToString();
		}
        #endregion
	}
}
