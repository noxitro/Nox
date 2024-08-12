using ClangSharp.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    public static class CppParseDefine
    {
        /// <summary>
        /// c++バージョン定義
        /// </summary>
        public enum CppVersion : byte
        { 
            Cpp20,
            LastTest,

            _Max
        }

        #region 定数
        #endregion

        #region 非公開フィールド
        private static readonly string[] CppVersionStrTable = new string[(int)CppVersion._Max]
       {
            "-std=c++20",
            "-std=c++2b",
       };
        #endregion

        #region 公開フィールド


        /// <summary>
        /// nitro::Object
        /// </summary>
        public const string NitroObjectFullName = "nitro::Object";
        public const string RexReflectionAttributeContainerClassName = "RexReflectionAttributeContainer";
        public const string NitroAttributeContainerMethod = "AttrContainer";


        public const string NitroTypeInfoRefHolderBase = "TypeInfoRefHolderBase";

        /// <summary>
        /// リフレクション公開用構造体名
        /// </summary>
        public const string NitroTypeInfoRefHolder = "TypeInfoRefHolder";
        #endregion

        #region 公開プロパティ

        #endregion

        #region 公開メソッド
        public static string GetCppVersionStr(CppVersion cppVersion)
        {
            return CppVersionStrTable[(int)cppVersion];
        }
        #endregion
    }
}
