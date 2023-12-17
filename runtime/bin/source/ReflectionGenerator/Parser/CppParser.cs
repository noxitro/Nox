using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Parser
{
    public unsafe class CppParser
    {
        #region 公開定義
        public class SetupParam
        {
            /// <summary>
            /// c++バージョン
            /// </summary>
            public CppParseDefine.CppVersion CppVersion { get; set; } = CppParseDefine.CppVersion.LastTest;

            /// <summary>
            /// 解析対象ファイルパス
            /// </summary>
            public string SourceFilePath { get; set; } = string.Empty;

            /// <summary>
            /// 解析対象外のroot namespaceリスト
            /// </summary>
            public List<string> IgnoreNamespaceList { get; set; } = new List<string>();

            /// <summary>
            /// 解析対象のnamespace
            /// nullの場合、全てが対象
            /// </summary>
            public List<string>? EnableRootNamespaceList { get; set; } = null;
        }
        #endregion

        #region 非公開フィールド
        private Info.NamespaceInfoStack _NamespaceInfoStack = new Info.NamespaceInfoStack();

        /// <summary>
        /// 無視するroot namespace list
        /// </summary>
        private string[] _IgnoreNamespaceList = new string[0];

        /// <summary>
        /// 解析対象のroot namespace list
        /// </summary>
        private string[]? _EnableRootNamespaceList = new string[0];

        /// <summary>
        /// 
        /// </summary>
        private string _ProjectRootDirectory = string.Empty;

        ///// <summary>
        ///// templateクラス、関数の解析を有効にするか
        ///// </summary>
        //private bool _EnableTemplate = true;

        /// <summary>
        /// templateクラス名リスト
        /// 最後に、変換を行うため保持
        /// </summary>
        private readonly Dictionary<string, Info.TemplatedClassInfo> _TemplatedClassNameList = new Dictionary<string, Info.TemplatedClassInfo>();
        #endregion

        #region 公開プロパティ
        /// <summary>
        /// Enum情報
        /// </summary>
        public Info.EnumInfoStack EnumInfoStack { get; } = new Info.EnumInfoStack();

        public Info.ClassInfoStack ClassInfoStack { get; } = new Info.ClassInfoStack();

        public Info.GlobalInfoContainer GlobalInfoContainer { get; } = new Info.GlobalInfoContainer();

        /// <summary>
        /// 属性リスト
        /// </summary>
        public Info.AttributeInfoStack AttributeInfoStack { get; } = new Info.AttributeInfoStack();

        public List<string> ModuleNameList { get; } = new List<string>();
        #endregion

        #region 公開メソッド
        /// <summary>
        /// 解析開始
        /// </summary>
        /// <param name="setupParam"></param>
        /// <returns></returns>
        public bool Parse(SetupParam setupParam)
        {
            //  引数
            List<string> parseCommandLineList = new List<string>();

            parseCommandLineList.Add(CppParseDefine.GetCppVersionStr(setupParam.CppVersion));

            ClangSharp.Interop.CXIndex cxIndex = ClangSharp.Interop.CXIndex.Create(true, true);
            ClangSharp.Interop.CXTranslationUnit transUnit = null;
            ClangSharp.Interop.CXTranslationUnit_Flags translationUnitFlags = ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_SkipFunctionBodies;

            //  コンパイル
            ClangSharp.Interop.CXErrorCode cxErrorCode = ClangSharp.Interop.CXTranslationUnit.TryParse(
             cxIndex,
             setupParam.SourceFilePath,
             parseCommandLineList.ToArray(),
             Array.Empty<ClangSharp.Interop.CXUnsavedFile>(),
             translationUnitFlags,
             out transUnit
             );

            if (cxErrorCode != ClangSharp.Interop.CXErrorCode.CXError_Success)
            {
                Trace.Error(this, "failed parse code");
                return false;
            }

            //  各種パラメータのセットアップ
            _EnableRootNamespaceList = setupParam.EnableRootNamespaceList?.ToArray();

            _IgnoreNamespaceList = setupParam.IgnoreNamespaceList.ToArray();

            //  解析開始
            transUnit.Cursor.VisitChildren(VisitChild, clientData: default);

            //  tempaltedクラスを生成する
           foreach(Info.TemplatedClassInfo temp in _TemplatedClassNameList.Values)
            {
                Info.ClassInfo? classInfo = ClassInfoStack.GetInfo(temp.DefinitionCursor);
                if(classInfo == null)
                {
                    continue;
                }

            }

            //  後処理


            System.Threading.Tasks.Parallel.ForEach(ClassInfoStack.InfoList,
                (Info.ClassInfo classInfo) =>
                {
                    Info.ClassInfo[] baseClassInfo = new Info.ClassInfo[classInfo.BaseClassNameList.Count];
                    for (int i = 0; i < classInfo.BaseClassNameList.Count; ++i)
                    {
                        Info.ClassInfo? temp = ClassInfoStack.GetInfo(classInfo.BaseClassNameList[i]);
                        System.Diagnostics.Debug.Assert(temp != null, $"クラスが見つかりませんでした:{classInfo.BaseClassNameList[i]}");
                        baseClassInfo[i] = temp;

                        if (baseClassInfo[i].IsFake == true)
                        {
                            System.Threading.Interlocked.Exchange(ref baseClassInfo[i].Fake, 1);
                        }
                    }
                    classInfo.BaseClassList = baseClassInfo;
                }
                );

            //  
            Trace.Info(this, "クラス列挙開始");
            foreach (var classInfo in ClassInfoStack.InfoList)
            {
                Trace.Info(this, $"\n\tClass\n\tNamespace:{classInfo.Namespace}\n\tFullName:{classInfo.FullName}\n\tName:{classInfo.Name}");

                foreach (var fieldInfo in classInfo.FieldInfoList)
                {
                    Trace.Info(this, $"\n\tField:{fieldInfo.Name}\n\tNamespace:{fieldInfo.RuntimeType.Namespace}\n\tFullName:{fieldInfo.RuntimeType.FullName}\n\tName:{fieldInfo.RuntimeType.Name}");
                }

                foreach (var methodInfo in classInfo.MethodInfoList)
                {
                    Trace.Info(this, $"\n\tMethod:{methodInfo.Name}\n\tNamespace:{methodInfo.ReturnRuntimeType.Namespace}\n\tFullName:{methodInfo.ReturnRuntimeType.FullName}\n\tName:{methodInfo.ReturnRuntimeType.Name}");
                }
            }

            return true;
        }
        #endregion

        #region 非公開メソッド
        private ClangSharp.Interop.CXChildVisitResult VisitChild(ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXCursor parent, void* client_data)
        {
            if (
                cursor.Spelling.CString.Contains("intValue") ||
                cursor.Spelling.CString.Contains("app::Int") 
                )
            {
            //    Trace.Info(this, "");
          //      cursor.VisitChildren(VisitChild, default);
            }

            if (
                cursor.Spelling.CString.Contains("asBody") ||
                parent.Spelling.CString.Contains("asValue") 
                )
            {
                Trace.Info(this, "");
         //       cursor.VisitChildren(VisitChild, default);
            }

            if (cursor.IsTemplated)
            {
                System.Diagnostics.Debug.Assert(cursor.NumTemplateParameterLists <= 1, $"未対応の識別値です: {cursor.Spelling.CString}");
            }

            

            switch (cursor.Kind)
            {
                //  namespace
                case ClangSharp.Interop.CXCursorKind.CXCursor_Namespace:
                    ParseNamespace(cursor);
                    break;

                //  class, struct, union
                case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_StructDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_UnionDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_CXXBaseSpecifier:
                case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplate:

                    ParseClassDecl(cursor);
                    break;

                //  templateは未対応

                case ClangSharp.Interop.CXCursorKind.CXCursor_FunctionTemplate:
                    break;

                //  関数
                case ClangSharp.Interop.CXCursorKind.CXCursor_CXXMethod:
                case ClangSharp.Interop.CXCursorKind.CXCursor_Constructor:
                case ClangSharp.Interop.CXCursorKind.CXCursor_FunctionDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_Destructor:
                    if (IsParseNamespace() == false)
                    {
                        break;
                    }
                    ParseMethod(cursor);
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_VarDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_FieldDecl:
                    if(IsParseNamespace() == false)
                    {
                        break;
                    }
                    ParseFieldDecl(cursor);
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_FirstDecl:
                    cursor.VisitChildren(VisitChild, default);
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_CompoundAssignOperator:
                case ClangSharp.Interop.CXCursorKind.CXCursor_ConditionalOperator:
               
                case ClangSharp.Interop.CXCursorKind.CXCursor_CXXNewExpr:
                    Trace.Info(this, $"Kind:{cursor.Kind.ToString()}");
                    System.Diagnostics.Debug.Assert(false, $"未対応の識別値です {cursor.Type.CanonicalType.KindSpelling.CString}");
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasDecl:
                    switch (cursor.Type.CanonicalType.kind)
                    {
                        case ClangSharp.Interop.CXTypeKind.CXType_Record:
                            ParseClassDecl(cursor);
                            break;

                        case ClangSharp.Interop.CXTypeKind.CXType_Unexposed:
                            break;

                        default:
                           // System.Diagnostics.Debug.Assert(false, $"未対応の識別値です {cursor.Type.CanonicalType.KindSpelling.CString}");
                            break;
                    }
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasTemplateDecl:
                    Trace.Info(this, "");
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_CompoundStmt:
                case ClangSharp.Interop.CXCursorKind.CXCursor_DeclStmt:
                    cursor.VisitChildren(VisitChild, default);
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_BinaryOperator:
                case ClangSharp.Interop.CXCursorKind.CXCursor_UnaryOperator:
                default:

                    break;

            }

            return ClangSharp.Interop.CXChildVisitResult.CXChildVisit_Continue;
        }

        private string GetIndent()
        {
            string indentStr = string.Empty;

            //  namespace
            for (int i = 0; i < _NamespaceInfoStack.InfoStack.Count; ++i)
            {
                indentStr = string.Format(indentStr, "\t");
            }

            //  class
            for (int i = 0; i < ClassInfoStack.InfoStack.Count; ++i)
            {
                indentStr = string.Format(indentStr, "\t");
            }

            return indentStr;
        }

        /// <summary>
        /// namespaceのscopeを取得
        /// </summary>
        /// <returns></returns>
        private string GetFullNamespaceScope()
        {
            string retNamespace = string.Empty;
            foreach (var info in _NamespaceInfoStack.InfoStack.Reverse())
            {
                if (retNamespace != string.Empty)
                {
                    retNamespace += "::";
                }
                retNamespace += info.Name;
            }

            return retNamespace;
        }

        private string GetFullClassScope()
        {
            string retNamespace = string.Empty;
            foreach (var info in ClassInfoStack.InfoStack.Reverse())
            {
                if (retNamespace != string.Empty)
                {
                    retNamespace += "::";
                }
                retNamespace += info.Name;
            }

            return retNamespace;
        }

        private string GetFullScope()
        {
            string retName = string.Empty;
            string namespaceNames = GetFullNamespaceScope();
            if (namespaceNames != string.Empty)
            {
                retName += namespaceNames;
            }

            string classNames = GetFullClassScope();
            if (classNames != string.Empty)
            {
                if (retName != string.Empty)
                {
                    retName += "::";
                }
                retName += classNames;
            }

            return retName;
        }

        /// <summary>
        /// モジュール情報を作成
        /// </summary>
        /// <param name="cursor"></param>
        /// <returns></returns>
        private Info.Module CreateModule(ClangSharp.Interop.CXCursor cursor)
        {
            //  ソリューションディレクトまで辿って、.vcxprojを探す
            //  なければ、unnamedとして扱う
            cursor.Location.GetFileLocation(out ClangSharp.Interop.CXFile outFile, out uint outLine, out uint outColumn, out uint outOffset);

            string? path = System.IO.Path.GetFullPath(outFile.Name.CString);

            string moduleName = "unknown";

            //  ルートディレクトリ内か？
            if (path.StartsWith(_ProjectRootDirectory) == true)
            {
                return new Info.Module() { ModuleName = moduleName };
            }

            while (path != null)
            {
                string[] vcxProjectFileList = Directory.GetFiles(path, "*.vcxproj");
                if (vcxProjectFileList.Length > 0)
                {
                    if (vcxProjectFileList.Length > 1)
                    {
                        Trace.Warning(this, $".vcxprojファイルが複数見つかりました　最初に見つかったファイルをmodule名として扱います\n{vcxProjectFileList}");
                    }

                    moduleName = Path.GetFileNameWithoutExtension(vcxProjectFileList[0]);
                    break;
                }

                if (_ProjectRootDirectory == path)
                {
                    break;
                }

                path = Path.GetDirectoryName(path);
            }

            Info.Module module = new Info.Module() { ModuleName = moduleName };

            //  作成したモジュールを追加
            if (ModuleNameList.Contains(moduleName) == false)
            {
                ModuleNameList.Add(moduleName);
            }

            return module;
        }

        /// <summary>
        /// テンプレートパラメータリストテーブルを作成する
        /// </summary>
        /// <param name="cursor"></param>
        /// <returns></returns>
        private Info.TemplateParam[][] CreateTemplateParamListTable(ClangSharp.Interop.CXCursor cursor)
        {
            if (cursor.IsTemplated == false)
            {
                return new Info.TemplateParam[0][];
            }

            int numTemplateParameterLists = cursor.NumTemplateParameterLists;
            Info.TemplateParam[][] templateParamListTable = new Info.TemplateParam[numTemplateParameterLists][];


            for (uint templateListIndex = 0; templateListIndex < numTemplateParameterLists; ++templateListIndex)
            {
                int numTemplateParam = cursor.GetNumTemplateParameters(templateListIndex);
                templateParamListTable[templateListIndex] = new Info.TemplateParam[numTemplateParam];

                for (uint templateIndex = 0; templateIndex < numTemplateParam; ++templateIndex)
                {
                    ClangSharp.Interop.CXCursor templateCursor = cursor.GetTemplateParameter(templateListIndex, templateIndex);

                    Info.TemplateParamType kind = templateCursor.Kind switch
                    {
                        ClangSharp.Interop.CXCursorKind.CXCursor_TemplateTypeParameter => Info.TemplateParamType.Tempalte,
                        ClangSharp.Interop.CXCursorKind.CXCursor_TemplateTemplateParameter => Info.TemplateParamType.TemplateTempalte,
                        ClangSharp.Interop.CXCursorKind.CXCursor_NonTypeTemplateParameter => Info.TemplateParamType.NonType,
                        _ => default
                    };

                    templateParamListTable[templateListIndex][templateIndex] = new Info.TemplateParam() { Kind = kind };

                    int numTemplateArgs = templateCursor.NumTemplateArguments;
                    for (uint templateArgsIndex = 0; templateArgsIndex < numTemplateArgs; ++templateArgsIndex)
                    {
                        ClangSharp.Interop.CX_TemplateArgument templateArg = templateCursor.GetTemplateArgument(templateArgsIndex);
                        Trace.Info(this, templateArg.AsType.Spelling.CString);
                    }
                }
            }

            return templateParamListTable;
        }

        
        /// <summary>
        /// 解析対象かどうかを判定
        /// </summary>
        /// <param name="cursor"></param>
        /// <returns></returns>
        private bool IsParseTarget(ClangSharp.Interop.CXCursor cursor)
        {
            string namespaceStr = cursor.GetNamespace();

            if (_IgnoreNamespaceList.Length > 0)
            {
                if (_IgnoreNamespaceList.Contains(namespaceStr) == true)
                {
                    return false;
                }
            }

            if (_EnableRootNamespaceList?.Length > 0)
            {
                if (_EnableRootNamespaceList.Contains(namespaceStr) == true)
                {
                    return true;
                }
            }

            return true;
        }

        private bool IsParseNamespace()
        {
            if (_IgnoreNamespaceList.Length > 0)
            {
                if (_IgnoreNamespaceList.Contains(GetFullNamespaceScope()) == true)
                {
                    return false;
                }
            }

            if (_EnableRootNamespaceList?.Length > 0)
            {
                if (_EnableRootNamespaceList.Contains(GetFullNamespaceScope()) == true)
                {
                    return true;
                }
            }

            return true;
        }
        #region パース関係
        private void ParseNamespace(ClangSharp.Interop.CXCursor cursor)
        {
            if (cursor.IsDefinition == false)
            {
                //  宣言のみはスルー
                //return;
            }

            string name = ClangSharp.Interop.clang.getCursorSpelling(cursor).CString;

            //  rootのnamespaceがparse対象か
            if (_NamespaceInfoStack.EmptyStack == false)
            {
                if (_EnableRootNamespaceList?.Contains(name) == false)
                {
                    return;
                }
            }

            if (_IgnoreNamespaceList.Contains(name) == true)
            {
                return;
            }

            Trace.Info(this, string.Format("{0}namespace:{1}", GetIndent(), ClangSharp.Interop.clang.getCursorSpelling(cursor).CString));

            _NamespaceInfoStack.Push(new Info.NamespaceInfo()
            {
                Name = ClangSharp.Interop.clang.getCursorSpelling(cursor).CString,
                IsInline = ClangSharp.Interop.clang.Cursor_isInlineNamespace(cursor) >= 1
            });


            GlobalInfoContainer.Push(GetFullNamespaceScope());

            cursor.VisitChildren(VisitChild, default);

            _NamespaceInfoStack.Pop();
            GlobalInfoContainer.Pop();
        }

        private Info.ClassInfo CreateClassInfo(ClangSharp.Interop.CXCursor cursor, IReadOnlyList<Info.AttributeInfo> attrList)
        {
            //  継承型
            string[] baseClassFullNameList = new string[int.Max(0, cursor.NumBases)];

            for (uint i = 0; i < baseClassFullNameList.Length; ++i)
            {
                ClangSharp.Interop.CXCursor baseCursor = cursor.GetBase(i);
                baseClassFullNameList[i] = baseCursor.Type.GetCanonicalTypeFullName();
            }

            //  
            Info.ClassInfo newClassInfo = new Info.ClassInfo()
            {
                Namespace = cursor.GetNamespace(),
                Module = CreateModule(cursor),
                CXCursor = cursor,
                Name = cursor.Type.GetCanonicalTypeName(),
                FullName = cursor.Type.GetCanonicalTypeFullName(),
                AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),

                IsTypedef = cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasDecl,
                AttributeInfoList = new List<Info.AttributeInfo>(attrList),
                BaseClassNameList = baseClassFullNameList,

                //  attributes
                IsTemplate = cursor.IsTemplated,
                IsRootClass = baseClassFullNameList.Length <= 0,
                IsFake = IsParseTarget(cursor) == false
            };

            //  別名定義情報
            if (cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasDecl)
            {
                newClassInfo.RecordClassFullName = cursor.Type.CanonicalType.Spelling.CString;
            }

            //  テンプレートの別名
            if(cursor.IsTemplated == false && cursor.Type.CanonicalType.NumTemplateArguments > 0 && (newClassInfo.IsFake == false || cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_CXXBaseSpecifier) )
            {
                if(_TemplatedClassNameList.ContainsKey(newClassInfo.FullName) == false)
                {
                    Info.TemplatedClassInfo.TemplateArgument[] templateArgumentList = new Info.TemplatedClassInfo.TemplateArgument[cursor.Type.CanonicalType.NumTemplateArguments];
                    for (uint i = 0; i <  templateArgumentList.Length; ++i)
                    {
                        ClangSharp.Interop.CX_TemplateArgument nativeArgumentInfo = cursor.Type.CanonicalType.GetTemplateArgument(i);
                        string str = string.Empty;
                        switch (nativeArgumentInfo.kind)
                        {
                            case ClangSharp.Interop.CXTemplateArgumentKind.CXTemplateArgumentKind_Type:
                                str = nativeArgumentInfo.AsType.CanonicalType.Spelling.CString;
                                break;

                            case ClangSharp.Interop.CXTemplateArgumentKind.CXTemplateArgumentKind_Template:
                                str = nativeArgumentInfo.AsTemplate.AsTemplateDecl.Type.CanonicalType.Spelling.CString;
                                break;

                            case ClangSharp.Interop.CXTemplateArgumentKind.CXTemplateArgumentKind_Integral:
                                str = nativeArgumentInfo.AsIntegral.ToString();
                                break;

                            default:
                                System.Diagnostics.Debug.Assert(false, $"未対応のTemplateArgumentKind:{nativeArgumentInfo.kind.ToString()}");
                                break;
                        }

                        templateArgumentList[i] = new Info.TemplatedClassInfo.TemplateArgument() { FullName = str };
                    }

                    _TemplatedClassNameList.Add(newClassInfo.FullName, new Info.TemplatedClassInfo()
                    {
                        FullName = newClassInfo.FullName,
                        Module = CreateModule(cursor),
                        DefinitionCursor = cursor.CanonicalCursor.Definition,
                        TemplateArgumentList = templateArgumentList
                    });
                }
            }

            return newClassInfo;
        }

        private void ParseClassDecl(ClangSharp.Interop.CXCursor cursor)
        {
            //  RexReflectionAttributeContainerか
            if (cursor.Spelling.CString.StartsWith(CppParseDefine.RexReflectionAttributeContainerClassName) == true)
            {
                AttributeInfoStack.Push(
                     new Info.AttributeInfo()
                     {
                         FullName = cursor.Type.CanonicalType.Spelling.CString,
                         IsConstexpr = cursor.IsConstexpr
                     }
                     );
            }
            else
            {
                //  前方宣言は無視
                if (cursor.IsDefinition == false && cursor.Kind != ClangSharp.Interop.CXCursorKind.CXCursor_CXXBaseSpecifier)
                {
                    cursor.VisitChildren(VisitChild, default);
                    return;
                }

                if (ClassInfoStack.Contains(cursor.Type.GetCanonicalTypeFullName()) == true)
                {
                    cursor.VisitChildren(VisitChild, default);
                    return;
                }

                Info.ClassInfo newClassInfo = CreateClassInfo(cursor, AttributeInfoStack.InfoList);
                AttributeInfoStack.Clear();

                ClassInfoStack.Push(newClassInfo);
                cursor.VisitChildren(VisitChild, default);
                ClassInfoStack.Pop();
            }
        }

        private void ParseMethod(ClangSharp.Interop.CXCursor cursor)
        {
            //  所属クラスがリフレクション対象でなければ、AccessLevel::Publicのみ収集する
            if (ClassInfoStack.EmptyStack == false)
            {
                if (ClassInfoStack.Current.IsReflection == false && cursor.CXXAccessSpecifier.GetAccessLevel() != AccessLevel.Public)
                {
                    return;
                }
            }

            //  属性
            if (AttributeInfoStack.EmptyStack == false)
            {
                System.Diagnostics.Debug.Assert(cursor.NumMethods == 1, $"属性クラスに関数が複数定義されています:{AttributeInfoStack.Current.FullName}");

                //  属性情報を取得する
                if (cursor.NumArguments == 1)
                {
                    cursor.VisitChildren(VisitChild, default);
                }
                return;
            }
            //  通常関数

            Info.MethodInfo.ArgInfo[] argInfoList = new Info.MethodInfo.ArgInfo[int.Max(0, cursor.IsTemplated == true ? cursor.NumTemplateArguments : cursor.NumArguments)];

            for (uint i = 0; i < argInfoList.Length; i++)
            {
                ClangSharp.Interop.CXCursor argumentCursor = cursor.GetArgument(i);
                argInfoList[i] = new Info.MethodInfo.ArgInfo()
                {
                    Name = argumentCursor.Spelling.CString,
                    RuntimeType = argumentCursor.CreateRuntimeType(cursor.Type),
                    CXTypeKind = argumentCursor.Type.kind,
                    HasDefaultValue = cursor.HasDefaultArg,
                };

            //    cursor.VisitChildren(VisitChild, default);
            }

            Info.MethodInfo methodInfo = new Info.MethodInfo() 
            { 
                Module = CreateModule(cursor),
                Name = cursor.Spelling.CString,
                AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                ReturnRuntimeType = cursor.CreateRuntimeType(cursor.ReturnType),

                IsConst = cursor.Type.IsConstQualified,
                IsInline = cursor.IsFunctionInlined,
                IsConstexpr = cursor.IsConstexpr,
                IsConstructor = cursor.IsConstructor(),
                IsVirtual = cursor.CXXMethod_IsVirtual,
                IsAbstract = cursor.CXXMethod_IsPureVirtual,
                IsStatic = cursor.CXXMethod_IsStatic,
                IsNoexcept = cursor.Type.ExceptionSpecificationType == ClangSharp.Interop.CXCursor_ExceptionSpecificationKind.CXCursor_ExceptionSpecificationKind_BasicNoexcept,

                ArgInfoList = argInfoList,
                AttributeInfoList = new List<Info.AttributeInfo>(AttributeInfoStack.InfoList),
            };

            AttributeInfoStack.Clear();

            //  クラスのメンバ
            if (ClassInfoStack.EmptyStack == false)
            {
                if (ClassInfoStack.Current.IsFake == false)
                {
                    ClassInfoStack.Current.MethodInfoList.Add(methodInfo);
                }
            }
            //  グローバル
            else
            {
                GlobalInfoContainer.Current?.MethodInfoList.Add(methodInfo);
            }

            cursor.VisitChildren(VisitChild, default);
        }

        private void ParseFieldDecl(ClangSharp.Interop.CXCursor cursor)
        {
            if (cursor.IsDefinition == false)
            {
                return;
            }

            if(cursor.IsTemplated)
            {
                Trace.Info(this, "");
            }

            if(cursor.Spelling.CString == "value")
            {
                Trace.Info(this, "");
            }

            Info.FieldInfo field = new Info.FieldInfo()
            {
                Module = CreateModule(cursor),
                Name = cursor.Spelling.CString,
                RuntimeType = cursor.CreateRuntimeType(cursor.Type),
                AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                IsConstexpr = cursor.IsConstexpr,
                IsTemplate = cursor.IsTemplated,
                AttributeInfoList = new List<Info.AttributeInfo>(AttributeInfoStack.InfoList)
            };

            //  属性の付与
            if (AttributeInfoStack.EmptyStack == false)
            {
                AttributeInfoStack.Clear();
            }

            //  クラスのメンバ
            if (ClassInfoStack.EmptyStack == false)
            {
                if (ClassInfoStack.Current.IsFake == false)
                {
                    ClassInfoStack.Current.FieldInfoList.Add(field);
                }
            }
            //  グローバル
            else
            {
                GlobalInfoContainer.Current?.FieldInfoList.Add(field);
            }

            //  変数の型がtemplateの別名定義なら、クラス情報を作成する
            if (cursor.IsTemplated == false
                && ClassInfoStack.Contains(field.RuntimeType.FullName) == false
                && cursor.Type.CanonicalType.Declaration.Kind != ClangSharp.Interop.CXCursorKind.CXCursor_NoDeclFound
                )
            {
                Info.ClassInfo classInfo = CreateClassInfo(cursor.Type.Declaration, new List<Info.AttributeInfo>());
                ClassInfoStack.Add(classInfo);
            }
        }
        #endregion
        #endregion
    }
}
