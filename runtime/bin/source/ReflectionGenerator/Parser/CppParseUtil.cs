using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Parser
{
    public static class CppParseUtil
    {
        #region 公開メソッド
        public static bool IsReflectionAttributeCursor(ClangSharp.Interop.CXCursor cursor)
        {
            if(cursor.Kind != ClangSharp.Interop.CXCursorKind.CXCursor_AnnotateAttr)
            {
                return false;
            }

            return cursor.Spelling.CString.Contains(CppParseDefine.REFLECTION_ATTRIBUTE_ANNOTATE_STR) == true;
        }

        public static string ExtractReflectionAttributeFromCursor(ClangSharp.Interop.CXCursor cursor)
        {
            string str = cursor.Spelling.CString.Substring(CppParseDefine.REFLECTION_ATTRIBUTE_ANNOTATE_STR.Length);
            return str;
        }
        #endregion
    }
}
