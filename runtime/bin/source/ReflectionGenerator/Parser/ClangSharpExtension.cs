using ClangSharp;
using ClangSharp.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Parser
{
    /// <summary>
    /// ClangSharpExtenti
    /// </summary>
    public static class ClangSharpExtension
    {
        #region 公開メソッド
         /// <summary>
        /// CXCursorから子CXCursorリストを取得する
        /// </summary>
        /// <param name="parent"></param>
        /// <returns></returns>
        public unsafe static IReadOnlyList<CXCursor> GetChildren(this ClangSharp.Interop.CXCursor parent, ClangSharp.Interop.CXChildVisitResult cxChildVisitResult = CXChildVisitResult.CXChildVisit_Continue)
        {
            List<CXCursor> cursors = new List<CXCursor>();
            parent.VisitChildren((cursor, parentCursor, clientData) =>
            {
                cursors.Add(cursor);
                return cxChildVisitResult;
            }, clientData: default);
            return cursors;
        }

        public static IReadOnlyList<CXCursor> GetDeclList(this ClangSharp.Interop.CXCursor self)
        {
            
            int numDecl = self.NumDecls;
            if(numDecl <= 0)
            {
                return [];
            }

            CXCursor[] cursors = new CXCursor[numDecl];

            for (uint i = 0; i < numDecl; ++i)
            {
                cursors[i] = self.GetDecl(i);
            }

            return cursors;
        }

        public static IReadOnlyList<CXCursor> GetSpecializationList(this ClangSharp.Interop.CXCursor self)
        {
            int numDecl = self.NumSpecializations;
            if (numDecl <= 0)
            {
                return [];
            }

            CXCursor[] cursors = new CXCursor[numDecl];

            for (uint i = 0; i < numDecl; ++i)
            {
                cursors[i] = self.GetSpecialization(i);
            }

            return cursors;
        }

        /// <summary>
        /// アクセスレベルを取得
        /// </summary>
        /// <param name="accessSpecifier"></param>
        /// <returns></returns>
        public static AccessLevel GetAccessLevel(this CX_CXXAccessSpecifier accessSpecifier)
        {
            switch (accessSpecifier)
            {
                case CX_CXXAccessSpecifier.CX_CXXPublic: return AccessLevel.Public;
                case CX_CXXAccessSpecifier.CX_CXXProtected: return AccessLevel.Protected;
                case CX_CXXAccessSpecifier.CX_CXXPrivate: return AccessLevel.Private;
                case CX_CXXAccessSpecifier.CX_CXXInvalidAccessSpecifier:
                    return AccessLevel.Public;
            }
            Trace.ErrorLine(null, string.Format("不明なCX_CXXAccessSpecifierです :{0}", accessSpecifier.ToString()));
            return AccessLevel.Public;
        }
        public static string GetBaseTypeName(this CXType cursor)
        {
            switch (cursor.kind)
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
                    return RuntimeTypeKind.Int8;

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

        public static bool IsConstructor(this CXCursor cursor)
        {
            if (
                clang.CXXConstructor_isConvertingConstructor(cursor) != 0 ||
                clang.CXXConstructor_isCopyConstructor(cursor) != 0 ||
                clang.CXXConstructor_isDefaultConstructor(cursor) != 0 ||
                clang.CXXConstructor_isMoveConstructor(cursor) != 0
                )
            {
                return true;
            }

            return false;
        }
        public static bool IsDefinedOperatorNew(this ClangSharp.Interop.CXCursor self)
        {
            return false;
        }

        public static IReadOnlyList<string> GetFullNameList(this ClangSharp.Interop.CXCursor cursor)
        {
            List<string> strList = new List<string>();
            while (cursor.IsNull == false)
            {
                switch (cursor.Kind)
                {
                    case CXCursorKind.CXCursor_ClassDecl:
                    case CXCursorKind.CXCursor_ClassTemplate:
                    case CXCursorKind.CXCursor_Namespace:
                    case CXCursorKind.CXCursor_StructDecl:
                    case CXCursorKind.CXCursor_FieldDecl:
                    case CXCursorKind.CXCursor_TypedefDecl:
                        strList.Add(cursor.Spelling.CString);
                        break;
                }

                cursor = cursor.SemanticParent;
            }

            return strList;
        }

        public static string GetFullName(this ClangSharp.Interop.CXCursor cursor)
        {
            ClangSharp.Interop.CXCursor parentCursor = cursor.SemanticParent;

            switch(cursor.Kind)
            {
                case CXCursorKind.CXCursor_FunctionDecl:
                case CXCursorKind.CXCursor_CXXMethod:
                case CXCursorKind.CXCursor_FieldDecl:
                case CXCursorKind.CXCursor_VarDecl:
                    {
                      
                    }
                    break;

                default:
                    if (parentCursor.Type.kind != CXTypeKind.CXType_Invalid)
                    {
                        return $"{parentCursor.Type.CanonicalType.Spelling.CString}::{cursor.Spelling.CString}";
                    }
                    break;
            }
            
            switch(parentCursor.Kind)
            {
                case CXCursorKind.CXCursor_Namespace:
                case CXCursorKind.CXCursor_NamespaceAlias:
                case CXCursorKind.CXCursor_NamespaceRef:
                    string fullName = cursor.Spelling.CString;
                    for (; parentCursor.Kind == CXCursorKind.CXCursor_Namespace; parentCursor = parentCursor.SemanticParent)
                    {
                        fullName = $"{parentCursor.Spelling.CString}::{fullName}";
                    }
                    return fullName;

                default:
                    return $"{parentCursor.Type.CanonicalType.Spelling.CString}::{cursor.Spelling.CString}";
            }

            //ClangSharp.Interop.CXCursor parentCursor = cursor.SemanticParent;
            //switch(parentCursor.Kind)
            //{
            //    case CXCursorKind.CXCursor_Namespace:
            //        return 
            //}

            //string str = string.Empty;
            //foreach (string s in GetFullNameList(cursor))
            //{
            //    if (str != string.Empty)
            //    {
            //        str = s + "::" + str;
            //    }
            //    else
            //    {
            //        str = s;
            //    }
            //}

            //return str;
        }

        public static string GetNamespace(this ClangSharp.Interop.CXCursor cursor)
        {
            string str = string.Empty;

            while (cursor != ClangSharp.Interop.CXCursor.Null)
            {
                if (cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_Namespace)
                {
                    if (str == string.Empty)
                    {
                        str = cursor.Spelling.CString;
                    }
                    else
                    {
                        str = $"{cursor.Spelling.CString}::{str}";
                    }
                }

                cursor = cursor.Referenced.SemanticParent;
            }

            return str;
        }

        public static string GetNamespace(this ClangSharp.Interop.CXType cxType)
        {
            return cxType.Declaration.GetNamespace();
        }

        public static List<string> GetNamespaceList(this ClangSharp.Interop.CXCursor cursor)
        {
            List<string> strList = new List<string>();

            while (cursor != ClangSharp.Interop.CXCursor.Null)
            {
                if (cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_Namespace)
                {
                    strList.Insert(0, cursor.Spelling.CString);
                }

                cursor = cursor.Referenced.SemanticParent;
            }

            return strList;
        }

        public static string GetLixicalsStr(this ClangSharp.Interop.CXCursor cursor)
        {
            string str = string.Empty;
            cursor = cursor.Referenced.LexicalParent;

            while (cursor != ClangSharp.Interop.CXCursor.Null)
            {
                switch(cursor.Kind)
                {
                    case CXCursorKind.CXCursor_ClassDecl:
                    case CXCursorKind.CXCursor_StructDecl:
                    case CXCursorKind.CXCursor_UnionDecl:
                        if (str == string.Empty)
                        {
                            str = cursor.Spelling.CString;
                        }
                        else
                        {
                            str = $"{cursor.Spelling.CString}::{str}";
                        }
                        break;

                    default:
                        break;
                }

                cursor = cursor.Referenced.LexicalParent;
            }

            return str;
        }

        /// <summary>
        /// 完全な型名を取得
        /// </summary>
        /// <param name="cursor"></param>
        /// <returns></returns>
        public static string GetCanonicalTypeFullName(this ClangSharp.Interop.CXType type)
        {
            ClangSharp.Interop.CXCursor cursor = type.CanonicalType.Declaration;

            if (cursor.Kind == CXCursorKind.CXCursor_NoDeclFound)
            {
                return type.CanonicalType.Spelling.CString;
            }

            string lexicalParentStr = GetLixicalsStr(cursor);
            string namespaceStr = GetNamespace(cursor);

            string str = type.CanonicalType.Spelling.CString;

            if (namespaceStr != string.Empty)
            {
                str = str.Replace($"{namespaceStr}::", string.Empty);
            }

            if (lexicalParentStr != string.Empty)
            {
                str = $"{lexicalParentStr}::{str}";
            }

            if (namespaceStr != string.Empty)
            {
                str = $"{namespaceStr}::{str}";
            }

            return str;
        }

       

        //public static string GetObjectFullName(this ClangSharp.Interop.CXCursor cursor)
        //{
        //    string lexicalParentStr = GetLixicalsStr(cursor);
        //    string namespaceStr = GetNamespace(cursor);

        //    string str = cursor.Spelling.CString;

        //    if (namespaceStr != string.Empty)
        //    {
        //        str = str.Replace($"{namespaceStr}::", string.Empty);
        //    }

        //    if (lexicalParentStr != string.Empty)
        //    {
        //        str = $"{lexicalParentStr}::{str}";
        //    }

        //    if (namespaceStr != string.Empty)
        //    {
        //        str = $"{namespaceStr}::{str}";
        //    }

        //    return str;
        //}

        //public static string GetFunctionFullName(this ClangSharp.Interop.CXCursor cursor)
        //{
        //    switch(cursor.Kind)
        //    {
        //        case CXCursorKind.CXCursor_FunctionDecl:
        //        case CXCursorKind.CXCursor_CXXMethod:
        //        case CXCursorKind.CXCursor_ObjCClassMethodDecl:
        //        case CXCursorKind.CXCursor_Constructor:
        //            break;

        //        default:
        //            return string.Empty;
        //    }

        //    string lexicalParentStr = GetLixicalsStr(cursor);
        //    string namespaceStr = GetNamespace(cursor);

        //    string str = cursor.Spelling.CString;

        //    if (namespaceStr != string.Empty)
        //    {
        //        str = str.Replace($"{namespaceStr}::", string.Empty);
        //    }

        //    if (lexicalParentStr != string.Empty)
        //    {
        //        str = $"{lexicalParentStr}::{str}";
        //    }

        //    if (namespaceStr != string.Empty)
        //    {
        //        str = $"{namespaceStr}::{str}";
        //    }

        //    return str;
        //}

        public static string GetCanonicalTypeName(this ClangSharp.Interop.CXType type)
        {
            System.Diagnostics.Debug.Assert(type.kind != CXTypeKind.CXType_Invalid, "無効な型");
            System.Diagnostics.Debug.Assert(type.CanonicalType.kind != CXTypeKind.CXType_Invalid, "無効な型");
            return type.CanonicalType.Spelling.CString;
        }

        public static ClangSharp.Interop.CXType GetDeepestPointeeType(this ClangSharp.Interop.CXType type)
        {
            while(type.PointeeType.kind != CXTypeKind.CXType_Invalid)
            {
                type = type.PointeeType;
            }

            return type;
        }

        private static string RestoreRuntimeTypeStr(RuntimeType runtimeType)
        {
            if (runtimeType.TemplateArgumentList.Count <= 0)
            {
                return runtimeType.FullName;
            }

            string str = runtimeType.FullName;

            str += "<";

            for(int i = 0; i < runtimeType.TemplateArgumentList.Count; ++i)
            {
                if(i < runtimeType.TemplateArgumentList.Count - 1)
                {
                    str += RestoreRuntimeTypeStr(runtimeType.TemplateArgumentList[i]) + ", ";
                }
                else
                {
                    str += RestoreRuntimeTypeStr(runtimeType.TemplateArgumentList[i]);
                }
            }
            str += ">";

            return str;
        }

        private static readonly HashSet<string> IgnorePropertyNameList = new HashSet<string> {
                "BinaryOperatorKindSpelling",
                "UnaryOperatorKindSpelling",
                //"TlsKind",
                //"LambdaStaticInvoker",
        };

        /// <summary>
        /// 除外するプロパティ名リスト
        /// </summary>
        private static bool IsIgnoreProperty(CXCursor cursor, string propertyName)
        {
            CXCursorKind cursorKind = cursor.Kind;
            CXTypeKind typeKind = cursor.Type.kind;
            CX_StorageClass storageClass = cursor.StorageClass;

            switch (propertyName)
            {
                case "DefaultArgType":
                    switch(cursorKind)
                    {
                        case CXCursorKind.CXCursor_TemplateTypeParameter:
                            switch(typeKind)
                            {

                            }
                            return true;
                    }
                    break;
                case "TlsKind":
                    switch(cursorKind)
                    {
                        case CXCursorKind.CXCursor_CompoundStmt:
                        case CXCursorKind.CXCursor_UnexposedExpr:
                        case CXCursorKind.CXCursor_IntegerLiteral:
                            switch (storageClass)
                            {
                                case CX_StorageClass.CX_SC_Invalid:
                                    return true;
                                //case CXTypeKind.CXType_Int:
                                //case CXTypeKind.CXType_ULongLong:
                                //    return true;
                            }
                            break;
                        case CXCursorKind.CXCursor_UnaryExpr:
                            switch(typeKind)
                            {
                                case CXTypeKind.CXType_ULongLong:
                                    return true;
                            }
                            break;
                        case CXCursorKind.CXCursor_ReturnStmt:
                        case CXCursorKind.CXCursor_CallExpr:
                        case CXCursorKind.CXCursor_DeclRefExpr:
                        case CXCursorKind.CXCursor_LambdaExpr:
                        case CXCursorKind.CXCursor_BinaryOperator:
                        case CXCursorKind.CXCursor_ParenExpr:
                        case CXCursorKind.CXCursor_UnaryOperator:
                        case CXCursorKind.CXCursor_FloatingLiteral:
                        case CXCursorKind.CXCursor_InvalidFile:
                        case CXCursorKind.CXCursor_CXXNullPtrLiteralExpr:
                        case CXCursorKind.CXCursor_CXXFunctionalCastExpr:
                        case CXCursorKind.CXCursor_MemberRefExpr:
                        case CXCursorKind.CXCursor_DeclStmt:
                        case CXCursorKind.CXCursor_StringLiteral:
                        case CXCursorKind.CXCursor_CStyleCastExpr:
                        case CXCursorKind.CXCursor_CXXThisExpr:
                            return true;
                    }
                    break;

                case "LambdaStaticInvoker":
                    switch(cursorKind)
                    {
                        case CXCursorKind.CXCursor_ClassDecl:
                            return true;
                    }
                    break;

                case "Visibility":
                    switch(cursorKind)
                    {
                        case CXCursorKind.CXCursor_LastExtraDecl:
                            return false;
                    }
                    break;

                case "DeclObjCTypeEncoding":
                    switch(cursorKind)
                    {
                        case CXCursorKind.CXCursor_ParmDecl:
                        case CXCursorKind.CXCursor_NonTypeTemplateParameter:
                        case CXCursorKind.CXCursor_VarDecl:
                        case CXCursorKind.CXCursor_ConversionFunction:
                        case CXCursorKind.CXCursor_CXXMethod:
                        case CXCursorKind.CXCursor_FieldDecl:
                            return true;
                    }
                    break;

                case "IsGlobal":
                    switch(cursorKind)
                    {
                        case CXCursorKind.CXCursor_NonTypeTemplateParameter:
                            return true;
                    }
                    break;
            }

            return false;
        }

        private static bool IsIgnoreProperty(CXType cxType, string propertyName)
        {


            return false;
        }


        public static List<(string Name, System.Type Type, object Value, string comment)> GetMemberInfoList(this object instance, bool checkCHildren = true)
        {
            CXCursor? cursor = instance as CXCursor?;

            
                List<(string Name, System.Type Type, object Value, string comment)> list = new List<(string Name, System.Type Type, object Value, string comment)>();

            if (cursor != null)
            {
                if (checkCHildren)
                {
                    var children = cursor.Value.GetChildren();
                    for (int i = 0; i < children.Count; ++i)
                    {
                        list.Add(($"Children[{i}]{children[i].Kind.ToString()}:{children[i].Spelling.CString}", children[i].GetType(), GetMemberInfoList(children[i]), string.Empty));
                    }
                }
            }

            System.Type type = instance.GetType();
            IReadOnlyList<System.Reflection.FieldInfo> fieldInfoList = type.GetFields((System.Reflection.BindingFlags)~0);
            foreach (System.Reflection.FieldInfo field in fieldInfoList)
            {
                //     Trace.Info(null, $"Field:{field.Name}");
                object? value = field.GetValue(instance);
                if (value == null)
                {
                    continue;
                }
                list.Add((field.Name, field.FieldType, value, string.Empty));
            }

            IReadOnlyList<System.Reflection.PropertyInfo> propertyInfoList = type.GetProperties((System.Reflection.BindingFlags)~0);
            foreach (System.Reflection.PropertyInfo propertyInfo in propertyInfoList)
            {
                if (IgnorePropertyNameList.Contains(propertyInfo.Name) == true)
                {
                    continue;
                }

                //  除外プロパティか

                if (cursor.HasValue == true)
                {
                    if (IsIgnoreProperty(cursor.Value, propertyInfo.Name) == true)
                    {
                        continue;
                    }
                }

                try
                {
                    switch (instance)
                    {
                        case ClangSharp.Interop.CXType cxType:
                            Trace.Info(null, $"Kind:{cxType.kind.ToString()}, ");
                            
                            break;

                        case ClangSharp.Interop.CXCursor cxCursor:
                            Trace.Info(null, $"Kind:{cxCursor.Kind.ToString()}, ");
                            break;

                        default:

                            break;
                    }

                    Trace.Info(null, $"Property:{propertyInfo.Name}, ");


                    object? value = propertyInfo.GetValue(instance);
                    if (value == null)
                    {
                        Trace.InfoLine(null, $"");
                        continue;
                    }

                    string comment;
                    switch (value)
                    {
                        case ClangSharp.Interop.CXType cxType:
                            comment = cxType.KindSpelling.CString;
                            break;

                        case ClangSharp.Interop.CXCursor cxCursor:
                            {
                                var property = cxCursor.GetType().GetProperty("DebuggerDisplayString", (System.Reflection.BindingFlags)~0);
                                string? propertyStr = (string?)property?.GetValue(cxCursor);
                                System.Diagnostics.Debug.Assert(propertyStr != null);
                                comment = propertyStr;

                            }
                            break;

                        default:
                            comment = value.ToString() ?? string.Empty;
                            break;
                    }

                    if (comment != string.Empty)
                    {
                        Trace.InfoLine(null, $"Value:{value.ToString()}[{comment}]");
                    }
                    else
                    {
                        Trace.InfoLine(null, $"Value:{value.ToString()}");
                    }


                    list.Add((propertyInfo.Name, propertyInfo.PropertyType, value, comment));

                   
                }
                catch (Exception)
                {
                    Trace.InfoLine(null, $"プロパティの取得に失敗:{propertyInfo.Name}");
                }
            }

            return list;
        }
         #endregion
    }
}
