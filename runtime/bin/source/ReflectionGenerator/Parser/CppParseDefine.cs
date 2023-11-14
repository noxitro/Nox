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

        #region 非公開フィールド
        private static readonly string[] CppVersionStrTable = new string[(int)CppVersion._Max]
       {
            "-std=c++20",
            "-std=c++latest",
       };
        #endregion

        #region 公開フィールド


        /// <summary>
        /// nitro::Object
        /// </summary>
        public const string NitroObjectFullName = "nitro::Object";
        public const string NitroAttributeFullName = "__NitroAttribute__";
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

        public static string GetBaseTypeName(this CXType cursor)
        {
            switch(cursor.kind)
            {
                case CXTypeKind.CXType_LValueReference:
                case CXTypeKind.CXType_RValueReference:
                case CXTypeKind.CXType_Pointer:
                    return cursor.PointeeType.Desugar.Spelling.CString;

            }

            return cursor.Spelling.CString;
		}

        public static RuntimeTypeKind GetRuntimeType(this CXType cxtype)
        {
            CXType canonicalType = cxtype.CanonicalType;

            switch (canonicalType.kind)
            {
                case CXTypeKind.CXType_Void:
                    return RuntimeTypeKind.Void;

                case CXTypeKind.CXType_Bool:
                    return RuntimeTypeKind.Bool;
                case CXTypeKind.CXType_UInt:
                    return RuntimeTypeKind.U64;
                case CXTypeKind.CXType_Long:
                case CXTypeKind.CXType_Int:
                    return RuntimeTypeKind.S32;

                case CXTypeKind.CXType_UChar:
                    return RuntimeTypeKind.U8;

                case CXTypeKind.CXType_UShort:
                    return RuntimeTypeKind.U16;
                case CXTypeKind.CXType_SChar:
                    return RuntimeTypeKind.S8;

                //case CXTypeKind.CXType_ULong
                case CXTypeKind.CXType_Unexposed:
                    return RuntimeTypeKind.Unexposed;
                case CXTypeKind.CXType_Pointer:
                case CXTypeKind.CXType_LongLong:
                case CXTypeKind.CXType_ULong:
                case CXTypeKind.CXType_ULongLong:
                case CXTypeKind.CXType_Float:
                case CXTypeKind.CXType_WChar:
                case CXTypeKind.CXType_LValueReference:
                case CXTypeKind.CXType_RValueReference:
                case CXTypeKind.CXType_Record:
                case CXTypeKind.CXType_Auto:
                case CXTypeKind.CXType_Enum:
                case CXTypeKind.CXType_Short:
                case CXTypeKind.CXType_Double:
                case CXTypeKind.CXType_Char16:
                case CXTypeKind.CXType_Char32:
                case CXTypeKind.CXType_Char_S:
                case CXTypeKind.CXType_Char_U:
                case CXTypeKind.CXType_FunctionProto:
                    break;

				default:
                    System.Diagnostics.Debug.Assert(false, $"不明なランタイムタイプ: {canonicalType.kind}");
                    break;
            }

            return RuntimeTypeKind.Invalid;
        }

        /// <summary>
        /// アクセスレベルを取得
        /// </summary>
        /// <param name="accessSpecifier"></param>
        /// <returns></returns>
        public static AccessLevel GetAccessLevel(this CX_CXXAccessSpecifier accessSpecifier)
        {
            switch(accessSpecifier)
            {
                case CX_CXXAccessSpecifier.CX_CXXPublic:    return AccessLevel.Public;
                case CX_CXXAccessSpecifier.CX_CXXProtected: return AccessLevel.Protected;
                case CX_CXXAccessSpecifier.CX_CXXPrivate:   return AccessLevel.Private;
                case CX_CXXAccessSpecifier.CX_CXXInvalidAccessSpecifier:
                    return AccessLevel.Public;
            }
            Trace.Error(null, string.Format("不明なCX_CXXAccessSpecifierです :{0}", accessSpecifier.ToString()));
            return AccessLevel.Public;
        }

        public static bool IsConstructor(this CXCursor cursor)
        {
            if(
                clang.CXXConstructor_isConvertingConstructor(cursor) != 0 ||
                clang.CXXConstructor_isCopyConstructor (cursor) != 0 ||
                clang.CXXConstructor_isDefaultConstructor(cursor) != 0 ||
                clang.CXXConstructor_isMoveConstructor(cursor) != 0 
                )
            {
                return true;
            }

            return false;
        }
        #endregion
    }
}
