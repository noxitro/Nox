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
        private string[] _EnableRootNamespaceList = new string[0];

        /// <summary>
        /// 
        /// </summary>
        private string _ProjectRootDirectory = string.Empty;

        ///// <summary>
        ///// templateクラス、関数の解析を有効にするか
        ///// </summary>
        //private bool _EnableTemplate = true;
        #endregion

        #region 公開プロパティ
        /// <summary>
        /// Enum情報
        /// </summary>
        public Info.EnumInfoStack EnumInfoStack { get; } = new Info.EnumInfoStack(); 

        public Info.ClassInfoStack ClassInfoStack { get; } = new Info.ClassInfoStack();

        public Info.GlobalInfoContainer GlobalInfoContainer { get;} = new Info.GlobalInfoContainer();

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
            if (setupParam.EnableRootNamespaceList != null)
            {
                _EnableRootNamespaceList = setupParam.EnableRootNamespaceList.ToArray();
            }
            _IgnoreNamespaceList = setupParam.IgnoreNamespaceList.ToArray();

            //  解析開始
            transUnit.Cursor.VisitChildren(VisitChild, clientData: default);

            return true;
        }
        #endregion

        #region 非公開メソッド
        private ClangSharp.Interop.CXChildVisitResult VisitChild(ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXCursor parent, void* client_data)
        {
            //  
            if(_EnableRootNamespaceList.Length > 0) 
            {
                if(_NamespaceInfoStack.EmptyStack == false)
                {
                    return ClangSharp.Interop.CXChildVisitResult.CXChildVisit_Continue;
                }
            }

            switch (cursor.Kind)
            {
                //  class, struct, union
                case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_StructDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_UnionDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplate:
                    ParseClassDecl(cursor);
                    break;

                //  関数
                case ClangSharp.Interop.CXCursorKind.CXCursor_CXXMethod:
                case ClangSharp.Interop.CXCursorKind.CXCursor_Constructor:
                case ClangSharp.Interop.CXCursorKind.CXCursor_FunctionDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_FunctionTemplate:
                case ClangSharp.Interop.CXCursorKind.CXCursor_Destructor:
                    ParseMethod(cursor);
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_BinaryOperator:
                case ClangSharp.Interop.CXCursorKind.CXCursor_CompoundAssignOperator:
                case ClangSharp.Interop.CXCursorKind.CXCursor_ConditionalOperator:
                case ClangSharp.Interop.CXCursorKind.CXCursor_UnaryOperator:
                case ClangSharp.Interop.CXCursorKind.CXCursor_CXXNewExpr:
                    Trace.Info(this, $"Kind:{cursor.Kind.ToString()}");
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
                if(vcxProjectFileList.Length > 0)
                {
                    if(vcxProjectFileList.Length > 1)
                    {
                        Trace.Warning(this, $".vcxprojファイルが複数見つかりました　最初に見つかったファイルをmodule名として扱います\n{vcxProjectFileList}");
                    }

                    moduleName = Path.GetFileNameWithoutExtension(vcxProjectFileList[0]);
                    break;
                }

                if(_ProjectRootDirectory == path)
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
            if (_NamespaceInfoStack.InfoStack.Count == 0)
            {
                if (_EnableRootNamespaceList.Contains(name) == false)
                {
                    return;
                }
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

        private Info.ClassInfo CreateClassInfo(ClangSharp.Interop.CXCursor cursor)
        {
            //  継承型
            int numBase = cursor.NumBases;
            string[] baseClassInfoList = new string[int.Max(0, numBase)];

            if (numBase > 0)
            {
                for (uint i = 0; i < numBase; ++i)
                {
                    ClangSharp.Interop.CXCursor baseCursor = cursor.GetBase(i);
                    baseClassInfoList[i] = baseCursor.Type.CanonicalType.Spelling.CString;
                }
            }

            //  継承クラスがtemplateクラスの場合、class情報を作成する


            //  nitro::Objectクラスを継承した親クラスを取得
            //  ClassInfo? parentNitroObject = Array.Find(baseClassInfoList, x => x?.IsNitroObjectClass == true);
            Info.ClassInfo newClassInfo = new Info.ClassInfo()
            {
                Module = CreateModule(cursor),
                CXCursor = cursor,
                Name = cursor.Spelling.CString,
                FullName = cursor.Type.CanonicalType.Spelling.CString,
                AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                IsTemplate = cursor.IsTemplated,
                //   ParentObjectClass = parentNitroObject,
                IsRootClass = baseClassInfoList.Length <= 0,
                //  IsNitroObjectClass = IsNitroObjectClass(baseClassInfoList),
                IsTypedef = cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasDecl,
                AttributeInfoList = new List<Info.AttributeInfo>(AttributeInfoStack.InfoList),
                BaseClassNameList = baseClassInfoList,
                TemplateParamListTable = CreateTemplateParamListTable(cursor)
            };

            AttributeInfoStack.Clear();

            //  別名定義情報
            if (cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasDecl)
            {
                newClassInfo.RecordClassFullName = cursor.Type.CanonicalType.Spelling.CString;
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
                Info.ClassInfo newClassInfo = CreateClassInfo(cursor);

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
            if(AttributeInfoStack.EmptyStack == false)
            {
                System.Diagnostics.Debug.Assert(cursor.NumMethods == 1, $"属性クラスに関数が複数定義されています:{AttributeInfoStack.Current.FullName}");

                cursor.VisitChildren(VisitChild, default);
            }
        }
        #endregion
        #endregion


    }
}
