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
        public static bool IsReflectionAttribute(this ClangSharp.Interop.CXCursor self) => CppParseUtil.IsReflectionAttributeCursor(self);
        public static string ExtractReflectionAttributeStr(this ClangSharp.Interop.CXCursor self) => CppParseUtil.ExtractReflectionAttributeFromCursor(self);

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

        public static string GetObjectFullName(this ClangSharp.Interop.CXCursor cursor)
        {
            string lexicalParentStr = GetLixicalsStr(cursor);
            string namespaceStr = GetNamespace(cursor);

            string str = cursor.Spelling.CString;

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

        public static string GetFunctionFullName(this ClangSharp.Interop.CXCursor cursor)
        {
            switch(cursor.Kind)
            {
                case CXCursorKind.CXCursor_FunctionDecl:
                case CXCursorKind.CXCursor_CXXMethod:
                case CXCursorKind.CXCursor_ObjCClassMethodDecl:
                case CXCursorKind.CXCursor_Constructor:
                    break;

                default:
                    return string.Empty;
            }

            string lexicalParentStr = GetLixicalsStr(cursor);
            string namespaceStr = GetNamespace(cursor);

            string str = cursor.Spelling.CString;

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

        public static string GetCanonicalTypeName(this ClangSharp.Interop.CXType type)
        {
            if (type.CanonicalType.Declaration.Kind == CXCursorKind.CXCursor_NoDeclFound)
            {
                return type.CanonicalType.Spelling.CString;
            }

            return type.CanonicalType.Declaration.Spelling.CString;
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

            }

            return false;
        }

        private static bool IsIgnoreProperty(CXType cxType, string propertyName)
        {


            return false;
        }


        public static List<(string Name, System.Type Type, object Value, string comment)> GetMemberInfoList(this object instance)
        {
            CXCursor? cursor = instance as CXCursor?;

            List<(string Name, System.Type Type, object Value, string comment)> list = new List<(string Name, System.Type Type, object Value, string comment)>();

            if(cursor != null)
            {
                var children = cursor.Value.GetChildren();
                for (int i = 0; i < children.Count; ++i)
                {
                    list.Add(($"Children[{i}]{children[i].Kind.ToString()}:{children[i].Spelling.CString}", children[i].GetType(), GetMemberInfoList(children[i]), string.Empty));
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
                            comment = cxCursor.KindSpelling.CString;
                            break;

                        default:
                            comment = string.Empty;
                            
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

        private unsafe static void CreateRuntimeValue2(RuntimeType parent, ClangSharp.Interop.CXCursor cursor)
        {
            RuntimeType runtimeType = RuntimeType.Null;
            switch (cursor.Kind)
            {
                case CXCursorKind.CXCursor_IntegerLiteral:
                    {
                        runtimeType = new RuntimeType()
                        {
                            FullName = cursor.Type.GetCanonicalTypeFullName(),
                            TemplateArgumentList = RuntimeType.DummyList,
                            TypeKind = cursor.Type.kind
                        };
                        runtimeType.TemplateValueStr = cursor.IntegerLiteralValue.ToString();

                        parent.RuntimeValueList.Add(runtimeType);
                    }
                    break;

                case CXCursorKind.CXCursor_StringLiteral:
                    {
                        runtimeType = new RuntimeType()
                        {
                            FullName = cursor.Type.GetCanonicalTypeFullName(),
                            TemplateArgumentList = RuntimeType.DummyList,
                            TypeKind = cursor.Type.kind
                        };
                        runtimeType.TemplateValueStr = cursor.StringLiteralValue.CString;

                        parent.RuntimeValueList.Add(runtimeType);
                    }
                    break;
                case CXCursorKind.CXCursor_InvalidFile:
                    break;

                case CXCursorKind.CXCursor_CallExpr:
                    {
                        if (cursor.IsImplicit == true)
                        {
                            break;
                        }

                        runtimeType = new RuntimeType()
                        {
                            Name = cursor.Definition.Type.GetCanonicalTypeName(),
                            FullName = cursor.Definition.Type.GetCanonicalTypeFullName(),
                            TemplateArgumentList = RuntimeType.DummyList,
                            TypeKind = cursor.Definition.Type.kind,
                            TemplateValueStr = cursor.Definition.GetFunctionFullName()
                        };

                        parent.RuntimeValueList.Add(runtimeType);

                        foreach (ClangSharp.Interop.CXCursor childCursor in cursor.GetChildren())
                        {
                            CreateRuntimeValue2(runtimeType, childCursor);
                        }
                    }
                    break;

                case CXCursorKind.CXCursor_TemplateRef:
                    foreach (ClangSharp.Interop.CXCursor childCursor in cursor.GetChildren())
                    {
                        CreateRuntimeValue2(parent, childCursor);
                    }
                    break;

                case CXCursorKind.CXCursor_CXXFunctionalCastExpr:
                    //runtimeType = new RuntimeType()
                    //{
                    //    Name = cursor.Type.GetCanonicalTypeName(),
                    //    FullName = cursor.Type.GetCanonicalTypeFullName(),
                    //    TemplateArgumentList = RuntimeType.DummyList,
                    //    TypeKind = cursor.Type.kind,
                    //    TemplateValueStr = cursor.Type.GetCanonicalTypeFullName()
                    //};
                    //parent.RuntimeValueList.Add(runtimeType);
                    foreach (ClangSharp.Interop.CXCursor childCursor in cursor.GetChildren())
                    {
                        CreateRuntimeValue2(parent, childCursor);
                    }
                    break;

                case CXCursorKind.CXCursor_Constructor:
                    {
                        runtimeType = new RuntimeType()
                        {
                            Name = cursor.Type.GetCanonicalTypeName(),
                            FullName = cursor.Type.GetCanonicalTypeFullName(),
                            TemplateArgumentList = RuntimeType.DummyList,
                            TypeKind = cursor.Type.kind,
                            TemplateValueStr = cursor.GetFunctionFullName()
                        };

                        parent.RuntimeValueList.Add(runtimeType);

                        foreach (ClangSharp.Interop.CXCursor childCursor in cursor.GetChildren())
                        {
                            CreateRuntimeValue2(runtimeType, childCursor);
                        }
                    }
                    break;

                case CXCursorKind.CXCursor_FunctionDecl:
                case CXCursorKind.CXCursor_CXXMethod:
                    if (cursor.IsImplicit == true)
                    {
                        break;
                    }

                    runtimeType = new RuntimeType()
                    {
                        Name = cursor.Type.GetCanonicalTypeName(),
                        FullName = cursor.Type.GetCanonicalTypeFullName(),
                        TemplateArgumentList = RuntimeType.DummyList,
                        TypeKind = cursor.Type.kind,
                        TemplateValueStr = cursor.GetFunctionFullName()
                    };

                    parent.RuntimeValueList.Add(runtimeType);

                    foreach (ClangSharp.Interop.CXCursor childCursor in cursor.GetChildren())
                    {
                        CreateRuntimeValue2(runtimeType, childCursor);
                    }
                    break;

                case CXCursorKind.CXCursor_DeclRefExpr:
                case CXCursorKind.CXCursor_TypeRef:
                    {
                        ClangSharp.Interop.CXCursor primaryTemplate = cursor.Definition.PrimaryTemplate;
                        if (primaryTemplate.IsTemplated == true)
                        {
                            int numTemplateArgument = primaryTemplate.NumTemplateArguments;
                            for (uint i = 0; i < numTemplateArgument; ++i)
                            {
                                ClangSharp.Interop.CX_TemplateArgument templateArgument = primaryTemplate.GetTemplateArgument(i);
                            }
                        }

                        foreach (ClangSharp.Interop.CXCursor childCursor in cursor.GetChildren())
                        {
                            CreateRuntimeValue2(parent, childCursor);
                        }
                    }
                    break;

                default:
                    foreach (ClangSharp.Interop.CXCursor childCursor in cursor.GetChildren())
                    {
                        CreateRuntimeValue2(parent, childCursor);
                    }
                    break;
            }

           
        }

        private unsafe static RuntimeType CreateRuntimeValue(ClangSharp.Interop.CXCursor cursor, int depth = 0)
        {
            
            string createSpace()
            {
                string space = string.Empty;
                for(int i = 0; i < depth; ++i)
                {
                    space += '\t';
                }
                return space;
            }

            RuntimeType runtimeValue;

            switch (cursor.Kind)
            {
                case CXCursorKind.CXCursor_IntegerLiteral:
                    Trace.InfoLine("CreateRuntimeValue", createSpace() + $"{cursor.Kind.ToString()}:{cursor.Spelling.CString}:{cursor.IntegerLiteralValue}");
                    break;

                case CXCursorKind.CXCursor_StringLiteral:
                    Trace.InfoLine("CreateRuntimeValue", createSpace() + $"{cursor.Kind.ToString()}:{cursor.Spelling.CString}:{cursor.StringLiteralValue}");
                    break;
                case CXCursorKind.CXCursor_InvalidFile:
                    Trace.InfoLine("CreateRuntimeValue", createSpace() + $"{cursor.Kind.ToString()}:{cursor.Spelling.CString}");
                    break;

                case CXCursorKind.CXCursor_CallExpr:
                    Trace.InfoLine("CreateRuntimeValue", createSpace() + $"{cursor.Kind.ToString()}:{cursor.Spelling.CString}");

                    break;

                case CXCursorKind.CXCursor_TemplateRef:
                    Trace.InfoLine("CreateRuntimeValue", createSpace() + $"{cursor.Kind.ToString()}:{cursor.Spelling.CString}");

                    {
                        int numTempalteArgument = cursor.NumTemplateArguments;
                        for(uint i = 0; i < numTempalteArgument; ++i)
                        {
                            var argument = cursor.GetTemplateArgument(i);
                            Trace.InfoLine("CreateRuntimeValue", $"\t{argument.kind.ToString()}:{argument.AsDecl.Spelling.CString}");
                        }
                    }
                    break;

                default:
                    Trace.InfoLine("CreateRuntimeValue", createSpace() + $"{cursor.Kind.ToString()}:{cursor.Spelling.CString}");
                    break;
            }
            ++depth;
            {
                int numTemplateArg = cursor.Type.CanonicalType.NumTemplateArguments;
                for (uint i = 0; i < numTemplateArg; ++i)
                {
                    var templateArgument = cursor.Type.CanonicalType.GetTemplateArgument(i);
                    Trace.InfoLine("CreateRuntimeValue", createSpace() + $"{templateArgument.kind.ToString()}:{templateArgument.ToString()}");
                }
            }

            {
                int numTemplateArg = cursor.NumTemplateArguments;
                for (uint i = 0; i < numTemplateArg; ++i)
                {
                    var templateArgument = cursor.GetTemplateArgument(i);
                    Trace.InfoLine("CreateRuntimeValue", createSpace() + $"{templateArgument.kind.ToString()}:{templateArgument.ToString()}");
                }
            }

            foreach (ClangSharp.Interop.CXCursor childCursor in cursor.GetChildren())
            {
                CreateRuntimeValue(childCursor, depth);
            }

            //if(cursor != cursor.Referenced && cursor.Referenced != ClangSharp.Interop.CXCursor.Null)
            //{
            //    foreach (ClangSharp.Interop.CXCursor childCursor in cursor.Referenced.GetChildren())
            //    {
            //        CreateRuntimeValue(childCursor, depth);
            //    }
            //}

            //if (cursor != cursor.Definition && cursor.Definition != ClangSharp.Interop.CXCursor.Null)
            //{
            //    foreach (ClangSharp.Interop.CXCursor childCursor in cursor.Definition.GetChildren())
            //    {
            //        CreateRuntimeValue(childCursor, depth);
            //    }
            //}

            switch (cursor.Kind)
            {
                case CXCursorKind.CXCursor_CallExpr:
                case CXCursorKind.CXCursor_CXXFunctionalCastExpr:
                    {
                        runtimeValue = RuntimeType.Null;
                        ClangSharp.Interop.CXCursor writtern = cursor.SubExprAsWritten;
                  //      runtimeValue = CreateRuntimeValue(writtern.Definition, writtern.Definition.Type);
                    }
                    break;

                case CXCursorKind.CXCursor_Constructor:
                    {
                        //int numArgument = cursor.NumArguments;
                        //for(uint i = 0; i < numArgument; ++i)
                        //{
                        //    ClangSharp.Interop.CXCursor argument = cursor.GetArgument(i);
                        //    CreateRuntimeValue(argument.Definition, argument.Definition.Type);
                        //}

                        

                        runtimeValue = RuntimeType.Null;
                    }
                    break;

                case CXCursorKind.CXCursor_ParmDecl:
                    runtimeValue = RuntimeType.Null;
                    break;

                default:
                    runtimeValue = RuntimeType.Null;
                //    Trace.Error(null, $"未対応の識別値です: {cursor.Spelling.CString}, Kind:{cursor.Kind.ToString()}");
                //    System.Diagnostics.Debug.Assert(false, $"未対応の識別値です: {cursor.Spelling.CString}, Kind:{cursor.Kind.ToString()}");

                    break;
            }

            return runtimeValue;
        }

        private unsafe static RuntimeType CreateRuntimeTypeImpl(ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXType type)
        {
            RuntimeType templateData;

            int numTemplateArg = type.CanonicalType.NumTemplateArguments;
            RuntimeType[] argList = new RuntimeType[int.Max(0, numTemplateArg)];

            switch (type.CanonicalType.TypeClass)
            {
                case CX_TypeClass.CX_TypeClass_TemplateTypeParm:
                    templateData = new RuntimeType
                    {
                        TemplateDepth = type.CanonicalType.Depth,
                        TemplateIndex = type.CanonicalType.Index,
                        TypeKind = type.CanonicalType.kind,
                        TemplateArgumentList = argList
                    };
                    break;

                default:
                    templateData = new RuntimeType
                    {
                        FullName = type.GetCanonicalTypeFullName(),
                        Name = type.GetCanonicalTypeName(),
                        Namespace = type.Declaration.GetNamespace(),
                        TypeKind = type.CanonicalType.kind,
                        TemplateArgumentList = argList
                    };
                    break;
            }

            if (numTemplateArg >= 0)
            {
                for(uint i = 0; i < numTemplateArg; ++i)
                {
                    ClangSharp.Interop.CX_TemplateArgument argument = type.CanonicalType.GetTemplateArgument(i);
                    switch (argument.kind)
                    {
                        case CXTemplateArgumentKind.CXTemplateArgumentKind_Type:
                            argList[i] = CreateRuntimeTypeImpl(cursor, argument.AsType);
                            break;

                        case CXTemplateArgumentKind.CXTemplateArgumentKind_Integral:
                            argList[i] = new RuntimeType() 
                            { 
                                Name = argument.IntegralType.GetCanonicalTypeName(),
                                Namespace = argument.IntegralType.Declaration.GetNamespace(),
                                FullName = argument.IntegralType.GetCanonicalTypeFullName(),
                                TypeKind = argument.IntegralType.kind,

                                TemplateValueStr = argument.AsIntegral.ToString(),
                                TemplateArgumentList = RuntimeType.DummyList,
                            };
                            break;

                        case CXTemplateArgumentKind.CXTemplateArgumentKind_Declaration:
                            //argument.AsDecl.NumSpecializations
                            var argumentCursor = cursor.GetChildren()[(int)i];
                            argList[i] = CreateRuntimeValue(argumentCursor);
                            CreateRuntimeValue2(templateData, argumentCursor);
                            break;

                        default:
                            System.Diagnostics.Debug.Assert(false, $"未対応の識別値です {type.Spelling.CString}");
                            break;
                    }
                }
            }

            

            return templateData;
        }

        //private static string RestoreTypeName(RuntimeType)
        //{

        //}

        public static RuntimeType CreateRuntimeType(this ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXType type)
        {
        //    if(type.Spelling.CString.Contains("Namespace00::Same2<decltype(1), decltype(nullptr)>"))
            if(type.Spelling.CString.Contains("asBody"))
            {
                Trace.InfoLine(null, "");
            }

            var r = CreateRuntimeTypeImpl(cursor, type);

            string restoreName = RestoreRuntimeTypeStr(r);

            return r;
        }
        #endregion
    }
}
