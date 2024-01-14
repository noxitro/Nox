using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    /// <summary>
    /// コードジェネレータ
    /// </summary>
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
            _Stream = new StreamWriter(path, false, Encoding.UTF8);

            //  ファイルをクリア
            _Stream.BaseStream.SetLength(0);

#if DEBUG
            _FilePath = path;
#endif
        }


        public void Dispose()
        {
            //  ネストのエラーチェック
            if (_NestList.Count > 0 || _ScopeStack.Count > 0)
            {
#if DEBUG
                Trace.Error("ネストが正しく記述されていません thisName: ", _FilePath);
#endif
            }

            _Stream.Close();
        }

        public void Close() => _Stream.Close();

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
        #endregion
    }

    public class CodeWriterNest : IDisposable
    {
        private readonly CodeWriter Writer_;

        public CodeWriterNest(CodeWriter writer_, CodeWriter.ScopeType scopeType)
        {
            Writer_ = writer_;
            Writer_.PushScope(scopeType);
        }

        public void Dispose()
        {
            Writer_.PopScope();
        }
    }
}
