using ClangSharp.Interop;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Numerics;
namespace ReflectionGenerator.Parser
{
    /// <summary>
    /// Cpp解析
    /// </summary>
    public unsafe class CppParser
    {
        #region 公開定義
        public struct SetupParam
        {
            /// <summary>
            /// 解析対象ファイルパス
            /// </summary>
            public required string SourceFilePath { get; init; }

            public required string SolutionPath { get; init; }

            /// <summary>
            /// プロジェクトファイルパス
            /// </summary>
            public required string ProjectFilePath { get; init; }

            /// <summary>
            /// プラットフォーム名
            /// </summary>
            public required string Platform { get; init; }

            /// <summary>
            /// ビルド構成
            /// </summary>
            public required string Configuration { get; init; }

            /// <summary>
            /// MSBuildのパス
            /// </summary>
            public required string MSBuildBinPath { get; init; }

            /// <summary>
            /// 解析対象外のroot namespaceリスト
            /// </summary>
            public required IReadOnlyList<string> IgnoreNamespaceList { get; init; }

            /// <summary>
            /// 解析対象のnamespace
            /// nullの場合、全てが対象
            /// </summary>
            public required IReadOnlyList<string> EnableRootNamespaceList { get; init; }
        }
        #endregion

        #region 非公開フィールド
        private Info.NamespaceInfoStack _NamespaceInfoStack = new Info.NamespaceInfoStack();

        /// <summary>
        /// 無視するroot namespace list
        /// </summary>
        private IReadOnlyList<string> _IgnoreNamespaceList = new string[0];


        /// <summary>
        /// 解析対象のroot namespace list
        /// </summary>
        private IReadOnlyList<string> _EnableRootNamespaceList = new string[0];

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

        /// <summary>
        /// テンプレートクラスを解析するかどうか
        /// </summary>
        private const bool _EnabledTemplateParse = false;

        private readonly Dictionary<int, string> _TypeNameDict = new Dictionary<int, string>();

        /// <summary>
        /// 型名を取得する
        /// キャッシュ対応
        /// </summary>
        /// <param name="cxType"></param>
        /// <returns></returns>
        private string GetTypeFullName(in ClangSharp.Interop.CXType cxType)
        {
            System.Diagnostics.Debug.Assert(cxType.kind != CXTypeKind.CXType_Invalid, "");

            int hash = cxType.GetHashCode();
            if (_TypeNameDict.TryGetValue(hash, out string? name) == false)
            {
                _TypeNameDict.Add(hash, name = cxType.CanonicalType.Spelling.CString);
            }

            return name;
        }

        /// <summary>
        /// ラムダ式を持つ型か
        /// </summary>
        /// <returns></returns>
        public bool HasLambdaType(in ClangSharp.Interop.CXType type)
        {
            string typeFullName = GetTypeFullName(type);
            return typeFullName.Contains("(lambda at ") == true;
        }
        #endregion

        #region 公開プロパティ
        /// <summary>
        /// モジュール名リスト
        /// </summary>
        public List<string> ModuleNameList { get; } = new List<string>();

        public Info.DeclHolder? RootDeclHolder { get; private set; } = null;
        private Dictionary<long, Info.IBaseInfo> TypeInfoDict { get; } = new Dictionary<long, Info.IBaseInfo>();

        private readonly Dictionary<string, Info.DeclHolder> _DeclHolderDict = new Dictionary<string, Info.DeclHolder>();
        #endregion

        #region 公開メソッド
        /// <summary>
        /// MSBuildを通して、コンパイルオプションを取得する
        /// </summary>
        /// <param name="outClangCommandLineList"></param>
        /// <param name="msbuildBinPath"></param>
        /// <param name="projectFilePath"></param>
        /// <param name="configuration"></param>
        /// <param name="platform"></param>
        /// <param name="sourceFilePath"></param>
        /// <returns></returns>
        private static bool parseBuildOptions(out List<string> outClangCommandLineList, string msbuildBinPath, string solutionPath, string configuration, string platform, string sourceFilePath)
        {
            outClangCommandLineList = new List<string>();

            if (System.IO.File.Exists(sourceFilePath) == false)
            {
                return false;
            }

             if (System.IO.File.Exists(solutionPath) == false)
            {
                Trace.ErrorLine(null, $"ソリューションファイルが存在しません:{solutionPath}");
                return false;
            }

       //     string outputFileBuild = "D:\\github\\Nox\\runtime\\bin\\source\\ReflectionGenerator";

            string outputFilePath = System.IO.Path.GetFullPath($"{System.IO.Path.GetTempPath()}/tmpMSBuild.log");
            
            string msbuildPath = System.IO.Path.GetFullPath($"{msbuildBinPath}/MSBuild.exe");
            string args = 
                $"/p:Configuration={configuration};Platform={platform} {solutionPath} /property:GenerateFullPaths=true /v:n /t:clean /t:ClCompile /p:SelectedFiles=\"{sourceFilePath}\" /p:PreprocessToFile=true /p:PreprocessOutput={outputFilePath}";

            if(System.IO.File.Exists(solutionPath.Replace("\"", "")) == false)
            {
                return false;
            }

            if(System.IO.File.Exists(msbuildPath) == false)
            {
                return false;
            }

            System.Diagnostics.Process process = new System.Diagnostics.Process();
            {
                System.Diagnostics.ProcessStartInfo startInfo = process.StartInfo;
                startInfo.FileName = msbuildPath;
                startInfo.Arguments = args;
                startInfo.Verb = "RunAs";
                startInfo.UseShellExecute = false;
                startInfo.RedirectStandardOutput = true;
                startInfo.StandardOutputEncoding = System.Text.Encoding.UTF8;
                startInfo.CreateNoWindow = true;
                startInfo.UseShellExecute = false;

                startInfo.RedirectStandardError = true;
            }

            System.Diagnostics.Stopwatch stopwatchProcess = new System.Diagnostics.Stopwatch();
            stopwatchProcess.Start();
            process.Start();
            process.OutputDataReceived += new System.Diagnostics.DataReceivedEventHandler((sender, e) => {
                if (!string.IsNullOrEmpty(e.Data))
                {
                    Console.WriteLine(e.Data);
                }
            });

            if (process.WaitForExit(1000) == false)
            {
               

       //         return false;
            }
            stopwatchProcess.Stop();
            Trace.InfoLine(null, $"MSBuildにかかった時間:{stopwatchProcess.ElapsedMilliseconds.ToString()}msec");

        //    string allStr = process.StandardOutput.ReadToEnd();

            //  CL.exeに渡している引数情報を取得する
            string clArgs = string.Empty;
            while (!process.StandardOutput.EndOfStream)
            {
                string? lineStr = process.StandardOutput.ReadLine();
                if(lineStr == null)
                {
                    break;
                }

                int index = lineStr.IndexOf("ClCompile:");
                if (index < 0)
                {
                    continue;
                }

                //  CL.exeの引数情報を取得
                clArgs = process.StandardOutput.ReadLine() ?? string.Empty;
                //                clArgs = lineStr.Substring(index + "CL.exe".Length);

                break;
            }

            string _ = process.StandardOutput.ReadToEnd();

            if (process.ExitCode != 0)
            {
                return false;
            }

            //       var regex = new System.Text.RegularExpressions.Regex(@"[\""].+?[\""]|[^ ]+");
            //      var spilitClArgs = regex.Match(clArgs);

            var splitClArgs = System.Text.RegularExpressions.Regex.Matches(clArgs, @"[\""].+?[\""]|[^ ]+")
                           .Cast<System.Text.RegularExpressions.Match>()
                           .Select(m => m.Value);

            string commandLineOptionStr = string.Empty;
            foreach(string splitArg in splitClArgs)
            {
                if(commandLineOptionStr != string.Empty)
                {
                    if (splitArg[0] != '/')
                    {
                        switch (commandLineOptionStr)
                        {
                            case "/D":
                                outClangCommandLineList.Add($"-D {splitArg}");
                                break;
                        }
                    }
                    commandLineOptionStr = string.Empty;
                }

                switch(splitArg)
                {
                    case "/D":
                        commandLineOptionStr = splitArg;
                        continue;

                    case "/MDd":
                        outClangCommandLineList.Add("-D _DEBUG");
                        continue;

                    case "/GR-":
                        outClangCommandLineList.Add("-fno-rtti");
                        continue;

                    //  最適化オプション
                    case "/Od":
                        outClangCommandLineList.Add("-O0");
                        continue;
                    case "/O1":
                        outClangCommandLineList.Add("-O1");
                        continue;
                    case "/O2":
                        outClangCommandLineList.Add("-O2");
                        continue;
                    case "/OX":
                        outClangCommandLineList.Add("-O3");
                        continue;
                }

                //  CPPバージョン
                {
                    int preIndex = splitArg.IndexOf("/std:c++");
                    if (preIndex >= 0)
                    {
                        string cppVersionStr = splitArg.Substring(preIndex + "/std:c++".Length);

                        if (cppVersionStr == "latest")
                        {
                            outClangCommandLineList.Add("-std=c++2b");
                        }
                        else
                        {
                            outClangCommandLineList.Add($"-std=c++{cppVersionStr}");
                        }
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// clangでパースするためのソースファイルを作成する
        /// </summary>
        /// <param name="solutionPath"></param>
        /// <returns></returns>
        public static string? CreateParseSourceFile(string solutionPath)
        {
            string parseSourceFilePath = System.IO.Path.GetFullPath($"{System.IO.Path.GetTempPath()}/tmpParseSource.cpp");

            if(System.IO.File.Exists(parseSourceFilePath) == true)
            {
                System.IO.File.Delete(parseSourceFilePath);
            }

            //  ソリューションファイルからプロジェクトファイルパスリストを取得する
            IReadOnlyList<string> projectFilePathList = ExtractVCXProjectFile.ExtractBuildOrderProjectPathList(solutionPath);

            using (System.IO.StreamWriter streamWriter = new System.IO.StreamWriter(parseSourceFilePath))
            {
                foreach (string projectFilePath in projectFilePathList)
                {
                    IReadOnlyList<string> headerFiles = ExtractVCXProjectFile.ExtractHeaderFiles(projectFilePath);
                    foreach (string headerFile in headerFiles)
                    {
                        streamWriter.WriteLine($"#include \"{headerFile}\"");
                    }
                }
            }

            return parseSourceFilePath;
        }

        /// <summary>
        /// 解析開始
        /// </summary>
        /// <param name="setupParam"></param>
        /// <returns></returns>
        public bool Parse(in SetupParam setupParam)
        {
            //  MSBuildを通して展開したコマンドラインを取得
            if (parseBuildOptions(
                out List<string> outCommandLineList,
                msbuildBinPath: setupParam.MSBuildBinPath,
                solutionPath: setupParam.ProjectFilePath,
                configuration: setupParam.Configuration,
                platform: setupParam.Platform,
                sourceFilePath:
                setupParam.SourceFilePath) == false)
            {
                return false;
            }

            {
                //  パースするソースファイルを作成
                string? parseSourceFilePath = CreateParseSourceFile(setupParam.SolutionPath);
                if(parseSourceFilePath == null)
                {
                    Trace.Error(this, "パース用ソースファイルの作成に失敗しました");
                    return false;
                }

                //  引数
                List<string> parseCommandLineList = new List<string>();

                //  MSBuildを通して展開したコマンドラインをセット
                parseCommandLineList.AddRange(outCommandLineList);

                //            parseCommandLineList.Add(CppParseDefine.GetCppVersionStr(setupParam.CppVersion));
                //  不明な属性を無視しない
                parseCommandLineList.Add($"-D {Define.RUNTIME_REFLECTION_GENERATOR_DEFINE}");
//                parseCommandLineList.Add($"-fmodule-file=my_module=my_module.pcm my_module.pcm -o my_module.out");

                ClangSharp.Interop.CXIndex cxIndex = ClangSharp.Interop.CXIndex.Create(true, true);
                ClangSharp.Interop.CXTranslationUnit transUnit;
                ClangSharp.Interop.CXErrorCode cxErrorCode;
                //  コンパイル
                //   for (int i = 0; i < 100; ++i)
                {
                    System.Diagnostics.Stopwatch stopwatchClangCompile = new System.Diagnostics.Stopwatch();
                    stopwatchClangCompile.Start();
                    cxErrorCode = ClangSharp.Interop.CXTranslationUnit.TryParse(
                     cxIndex,
                     parseSourceFilePath,
                     parseCommandLineList.ToArray(),
                     default,
                  //    ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_SkipFunctionBodies,
                    ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_None,
                     out transUnit
                     );

                    stopwatchClangCompile.Stop();
                    Trace.InfoLine(this, $"Clang Compile:{stopwatchClangCompile.ElapsedMilliseconds.ToString()}msec");
                }

                if (cxErrorCode != ClangSharp.Interop.CXErrorCode.CXError_Success)
                {
                    Trace.ErrorLine(this, "failed parse");
                    return false;
                }

                //  ビルドエラーの解析
                {
                    bool isSuccess = true;
                    for (uint i = 0; i < transUnit.NumDiagnostics; ++i)
                    {
                        ClangSharp.Interop.CXDiagnostic diagnostic = transUnit.GetDiagnostic(i);
                        switch (diagnostic.Severity)
                        {
                            case CXDiagnosticSeverity.CXDiagnostic_Error:
                            case CXDiagnosticSeverity.CXDiagnostic_Fatal:
                                Trace.ErrorLine(this, diagnostic.Format(ClangSharp.Interop.CXDiagnosticDisplayOptions.CXDiagnostic_DisplaySourceLocation).CString);
                                isSuccess = false;
                                return false;
                            case CXDiagnosticSeverity.CXDiagnostic_Warning:
                                Trace.WarningLine(this, diagnostic.Format(ClangSharp.Interop.CXDiagnosticDisplayOptions.CXDiagnostic_DisplaySourceLocation).CString);
                                break;

                            case CXDiagnosticSeverity.CXDiagnostic_Note:
                            case CXDiagnosticSeverity.CXDiagnostic_Ignored:
                                Trace.InfoLine(this, diagnostic.Format(ClangSharp.Interop.CXDiagnosticDisplayOptions.CXDiagnostic_DisplaySourceLocation).CString);
                                break;
                        }
                    }

                    if(isSuccess == false)
                    {
                        Trace.ErrorLine(this, "clang compile結果にError, Faitalが発生しています ログを確認してください");
                        return false;
                    }
                }

                //  各種パラメータのセットアップ
                _EnableRootNamespaceList = setupParam.EnableRootNamespaceList;

                _IgnoreNamespaceList = setupParam.IgnoreNamespaceList;

                //  解析開始
                //  Data rootParam = new Data() { Kind = Kind.Class, Parent = null };
                //  transUnit.Cursor.VisitChildren(VisitChild, clientData: (CXClientData)System.Runtime.CompilerServices.Unsafe.AsPointer(ref rootParam));
                ParseRoot(transUnit.Cursor);

                transUnit.Dispose();
                cxIndex.Dispose();
            }

            //  namespaceを結合

            Util.BreakPoint();
            //  列挙
            foreach(var v in TypeInfoDict.Values)
            {
                string? name = v.ToString();
                if (name == null)
                {
                    continue;
                }

                Trace.InfoLine(this, name);
            }

            return true;
        }
        #endregion

        #region 共有関数

        #endregion

        #region 非公開メソッド
        private Info.IHolder FindParseParent(ClangSharp.Interop.CXCursor cursor)
        {
            ClangSharp.Interop.CXCursor parent = cursor.SemanticParent;
            while (parent.IsInjectedClassName)
            {
                parent = parent.SemanticParent;
            }
            while(parent.Kind == CXCursorKind.CXCursor_LinkageSpec)
            {
                parent = parent.SemanticParent;
            }
            long hash = parent.Hash;

            if (TypeInfoDict.TryGetValue(hash, out Info.IBaseInfo? parentBaseInfo) == true)
            {
                return (Info.IHolder)parentBaseInfo;
            }

            ParseCursor(parent);
            return (Info.IHolder)TypeInfoDict[hash];
        }

        private T FindParseParent<T>(ClangSharp.Interop.CXCursor cursor) where T : Info.IHolder
            => (T)FindParseParent(cursor);

        /// <summary>
        /// 属性情報の取得
        /// </summary>
        /// <param name="cursor"></param>
        /// <returns></returns>
        private IReadOnlyList<Info.AttributeInfo> GetCustomAttributeList(ClangSharp.Interop.CXCursor cursor)
        {
            int numAttr = cursor.NumAttrs;
            if (numAttr <= 0)
            {
                return [];
            }

            System.Diagnostics.Debug.Assert(cursor.HasAttrs, "not has attrs");

            bool hasEngineAnnotateAttribute = false;

            Info.AttributeInfo[] attrList = new Info.AttributeInfo[numAttr];
            for (uint i = 0; i < numAttr; ++i)
            {
                ClangSharp.Interop.CXCursor attrCursor = cursor.GetAttr(i);

                if (attrCursor.Kind != CXCursorKind.CXCursor_UnexposedAttr)
                {
           //         Trace.InfoLine(this, attrCursor.KindSpelling.CString);
                }

                switch(attrCursor.Kind)
                {
                    case CXCursorKind.CXCursor_AnnotateAttr:
                        //  エンジン外のclang::annoate属性
                        if(attrCursor.Spelling.CString == Define.RUNTIME_REFLECTION_GENERATOR_DEFINE)
                        {
                            hasEngineAnnotateAttribute = true;
                            attrList[i] = new Info.AttributeInfo()
                            {
                                AttrKind = attrCursor.AttrKind,
                            };
                            break;
                        }

                        if(hasEngineAnnotateAttribute == false)
                        {
                            break;
                        }

                        attrList[i] = new Info.EngineAnnotateAttribute()
                        {
                            AttrKind = attrCursor.AttrKind,
                            Value = attrCursor.Spelling.CString,
                            IsConstexpr = true
                        };
                        break;

                    default:
                        attrList[i] = new Info.AttributeInfo()
                        {
                            AttrKind = attrCursor.AttrKind,
                        };
                        break;
                }
            }

            return attrList;
        }

        private T TryCreateBaseInfo<T>(long hash, Func<T> action, Action<T>? post = null) where T : Info.IBaseInfo
        {
            if (TypeInfoDict.TryGetValue(hash, out Info.IBaseInfo? baseInfo) == true)
            {
                return (T)baseInfo;
            }

            if(TypeInfoDict.ContainsKey(hash) == true)
            {
                System.Diagnostics.Debug.Assert(false);
            }

            T v = action();
            TypeInfoDict.Add(hash, v);
            post?.Invoke(v);
            return v;
        }

        private static ClangSharp.Interop.CXType GetTypeWithQualifiersRemoved(ClangSharp.Interop.CXType type)
        {
            switch(type.TypeClass)
            {
                case CX_TypeClass.CX_TypeClass_Decltype:
                    break;
            }

            switch(type.kind)
            {
                case CXTypeKind.CXType_Pointer:
                case CXTypeKind.CXType_LValueReference:
                case CXTypeKind.CXType_RValueReference:
                    return GetTypeWithQualifiersRemoved(type.PointeeType);
            }

            return type;
        }

        #region Parse
        private void ParseEnum(ClangSharp.Interop.CXCursor cursor)
        {
            Info.EnumInfo Create()
            {
                int numEnum = cursor.NumEnumerators;
                Info.EnumInfo.EnumVariable[] enumVariableList = numEnum > 0 ? new Info.EnumInfo.EnumVariable[numEnum] : [];

                for (uint i = 0; i < numEnum; ++i)
                {
                    ClangSharp.Interop.CXCursor enumCursor = cursor.GetEnumerator(i);

                    Info.Integer64 integer64;
                    if (enumCursor.IsUnsigned)
                    {
                        integer64 = new Info.Integer64() { UInt64 = enumCursor.EnumConstantDeclUnsignedValue };
                    }
                    else
                    {
                        integer64 = new Info.Integer64() { Int64 = enumCursor.EnumConstantDeclValue };
                    }
                    enumVariableList[i] = new Info.EnumInfo.EnumVariable()
                    {
                        TypeKind = enumCursor.Type.kind,
                        Integer64 = integer64,
                        AttributeInfoList = GetCustomAttributeList(cursor),
                        Name = enumCursor.Name.CString
                    };
                }

                Info.EnumInfo enumInfo = new Info.EnumInfo()
                {
                    Name = cursor.Spelling.CString,
                    AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                    Namespace = cursor.GetNamespace(),
                    FullName = cursor.GetFullName(),
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    CXType = cursor.Type,
                    VariableList = enumVariableList,
                };

                return enumInfo;
            }

            uint hash = cursor.Hash;
            Info.EnumInfo enumInfo = TryCreateBaseInfo(hash, Create, default);

            Info.IHolder parent = FindParseParent(cursor);
            parent.EnumInfoList.Add(enumInfo);
        }

        private void ParseTypedef(ClangSharp.Interop.CXCursor cursor)
        {
            ClangSharp.Interop.CXType type = cursor.TypedefDeclUnderlyingType.CanonicalType;
            ParseType(type);

            ClangSharp.Interop.CXType typeWithQualifiersRemoved = GetTypeWithQualifiersRemoved(type).CanonicalType;
            ParseType(typeWithQualifiersRemoved);

            CXType cursorType = cursor.Type.CanonicalType;
            System.Diagnostics.Debug.Assert(cursorType.kind != CXTypeKind.CXType_Invalid, "CXTypeの取得に失敗しました");

            long hash;
            switch(cursorType.TypeClass)
            {
                case CX_TypeClass.CX_TypeClass_Builtin:
                case CX_TypeClass.CX_TypeClass_FunctionProto:
                    hash = cursorType.GetHashCode();
                    break;

                default:
                    hash = cursor.Hash;
                    break;
            }

            if (TypeInfoDict.TryGetValue(hash, out Info.IBaseInfo? baseInfo) == true)
            {
                Info.TypeInfo typeInfo = (Info.TypeInfo)baseInfo;
                //  単純な別名
                if (type == typeWithQualifiersRemoved)
                {
                    typeInfo.TypeAliasNameHashSet.TryAdd(cursor.GetFullName());
                }
                //  修飾子付きの型の別名
                else
                {
                    if (typeInfo.TypeQualifiersInfoHashSet.TryGetValue(type.Spelling.CString, out HashSet<string>? nameHashSet) == false)
                    {
                        typeInfo.TypeQualifiersInfoHashSet.Add(type.Spelling.CString, nameHashSet = new HashSet<string>());
                    }
                    nameHashSet.Add(cursor.GetFullName());
                }
            }
        }

        private void ParseTypedef2(ClangSharp.Interop.CXCursor cursor)
        {
            ClangSharp.Interop.CXType type = cursor.TypedefDeclUnderlyingType.CanonicalType;
            ParseType(type);
            
            ClangSharp.Interop.CXType typeWithQualifiersRemoved = GetTypeWithQualifiersRemoved(type).CanonicalType;
            switch (typeWithQualifiersRemoved.TypeClass)
            {
                case CX_TypeClass.CX_TypeClass_SubstTemplateTypeParm:
                    typeWithQualifiersRemoved = typeWithQualifiersRemoved.CanonicalType;
                    break;
            }
            ParseType(typeWithQualifiersRemoved);
            

            Info.TypeInfo typeInfo;
            switch(typeWithQualifiersRemoved.TypeClass)
            {
                case CX_TypeClass.CX_TypeClass_Builtin:
                case CX_TypeClass.CX_TypeClass_Decltype:
                    {
                        if (TypeInfoDict.TryGetValue(typeWithQualifiersRemoved.GetHashCode(), out Info.IBaseInfo? baseInfo) == true)
                        {
                            typeInfo = (Info.TypeInfo)baseInfo;

                            //  単純な別名
                            if (type == typeWithQualifiersRemoved)
                            {
                                typeInfo.TypeAliasNameHashSet.TryAdd(cursor.TypedefDeclUnderlyingType.CanonicalType.Spelling.CString);
                            }
                            //  修飾子付きの型の別名
                            else
                            {
                                if (typeInfo.TypeQualifiersInfoHashSet.TryGetValue(type.Spelling.CString, out HashSet<string>? nameHashSet) == false)
                                {
                                    typeInfo.TypeQualifiersInfoHashSet.Add(type.Spelling.CString, nameHashSet = new HashSet<string>());
                                }
                                nameHashSet.Add(cursor.GetFullName());
                            }
                        }
                        else
                        {
                            System.Diagnostics.Debug.Assert(false, "");
                        }
                    }
                    break;

           
                default:
                    switch(typeWithQualifiersRemoved.Declaration.Kind)
                    {
                        case CXCursorKind.CXCursor_TypeAliasTemplateDecl:
                            {
                                if (TypeInfoDict.TryGetValue(typeWithQualifiersRemoved.Declaration.Hash, out Info.IBaseInfo? baseInfo) == true)
                                {
                                    typeInfo = (Info.TypeInfo)baseInfo;
                                    //  単純な別名
                                    if (type == typeWithQualifiersRemoved)
                                    {
                                        typeInfo.TypeAliasNameHashSet.TryAdd(cursor.GetFullName());
                                    }
                                    //  修飾子付きの型の別名
                                    else
                                    {
                                        if (typeInfo.TypeQualifiersInfoHashSet.TryGetValue(type.Spelling.CString, out HashSet<string>? nameHashSet) == false)
                                        {
                                            typeInfo.TypeQualifiersInfoHashSet.Add(type.Spelling.CString, nameHashSet = new HashSet<string>());
                                        }
                                        nameHashSet.Add(cursor.GetFullName());
                                    }
                                }
                                else
                                {
                                    break;
//                                    System.Diagnostics.Debug.Assert(false, "");
                                }
                            }
                            break;

                        default:
                            {
                                if (TypeInfoDict.TryGetValue(typeWithQualifiersRemoved.Declaration.Hash, out Info.IBaseInfo? baseInfo) == true)
                                {
                                    typeInfo = (Info.TypeInfo)baseInfo;
                                    //  単純な別名
                                    if (type == typeWithQualifiersRemoved)
                                    {
                                        typeInfo.TypeAliasNameHashSet.TryAdd(cursor.GetFullName());
                                    }
                                    //  修飾子付きの型の別名
                                    else
                                    {
                                        if (typeInfo.TypeQualifiersInfoHashSet.TryGetValue(type.Spelling.CString, out HashSet<string>? nameHashSet) == false)
                                        {
                                            typeInfo.TypeQualifiersInfoHashSet.Add(type.Spelling.CString, nameHashSet = new HashSet<string>());
                                        }
                                        nameHashSet.Add(cursor.GetFullName());
                                    }
                                }
                                else
                                {
                                    ParseCursor(typeWithQualifiersRemoved.Declaration);
                                    System.Diagnostics.Debug.Assert(false, "");
                                }
                            }
                            break;
                    }
                    break;
            }

        }

        private void ParseTemplateTypedef(ClangSharp.Interop.CXCursor cursor)
        {
        }

        private void ParseType(ClangSharp.Interop.CXType cxType)
        {
            switch(cxType.TypeClass)
            {
                case CX_TypeClass.CX_TypeClass_BlockPointer:
                case CX_TypeClass.CX_TypeClass_Builtin:
                    {
                        int hash = cxType.GetHashCode();

                        TryCreateBaseInfo(hash, () =>
                        {
                            return new Info.TypeInfo()
                            {
                                AccessLevel = AccessLevel.Public,
                                CXType = cxType,
                                FullName = cxType.Spelling.CString,
                                Namespace = string.Empty,
                                AttributeInfoList = []
                            };

                        }, null);
                    }
                    return;

                case CX_TypeClass.CX_TypeClass_Decltype:
                    {
                        int hash = cxType.GetHashCode();

                        TryCreateBaseInfo(hash, () =>
                        {
                            return new Info.TypeInfo()
                            {
                                AccessLevel = AccessLevel.Public,
                                CXType = cxType,
                                FullName = cxType.Spelling.CString,
                                Namespace = string.Empty,
                                AttributeInfoList = []
                            };

                        }, null);
                    }
                    return;

                case CX_TypeClass.CX_TypeClass_FunctionNoProto:
                case CX_TypeClass.CX_TypeClass_FunctionProto:
                    {
                        int hash = cxType.GetHashCode();

                        TryCreateBaseInfo(hash, () =>
                        {
                            return new Info.TypeInfo()
                            {
                                AccessLevel = AccessLevel.Public,
                                CXType = cxType,
                                FullName = cxType.Spelling.CString,
                                Namespace = string.Empty,
                                AttributeInfoList = []
                            };

                        }, null);
                    }
                    return;

                case CX_TypeClass.CX_TypeClass_Enum:
                    ParseCursor(cxType.Declaration);
                    return;

                case CX_TypeClass.CX_TypeClass_Auto:
                    //  autoは解析できないのでスキップ
                    return;
            }

            switch (cxType.kind)
            {
                case CXTypeKind.CXType_Elaborated:
                    ParseCursor(cxType.Declaration);
                    return;

                case CXTypeKind.CXType_Pointer:
                case CXTypeKind.CXType_RValueReference:
                case CXTypeKind.CXType_LValueReference:
                    ParseType(cxType.PointeeType);
                    return;


                case CXTypeKind.CXType_Record:
                    ParseCursor(cxType.Declaration);
                    return;

                case CXTypeKind.CXType_DependentSizedArray:
                    ParseType(cxType.ElementType);
                    return;

                case CXTypeKind.CXType_ConstantArray:
                    ParseType(cxType.ElementType);
                    return;

                case CXTypeKind.CXType_Unexposed:
                    return;
            }

            System.Diagnostics.Debug.Assert(false, "");
        }

        //private static string GetTypeFullName(ClangSharp.Interop.CXType cxType)
        //{
        //    string str = string.Empty;

        //    switch (cxType.TypeClass)
        //    {
        //        case CX_TypeClass.CX_TypeClass_Builtin:
        //            return cxType.Spelling.CString;

        //        case CX_TypeClass.CX_TypeClass_Record:
        //            ClangSharp.Interop.CXCursor lambdaCallOperator = cxType.Declaration.LambdaCallOperator;
        //            if (lambdaCallOperator.IsNull == false)
        //            {
        //                return lambdaCallOperator.ReturnType.CanonicalType.Spelling.CString;
        //            }

        //            return GetTypeFullName(cxType.Declaration);
        //    }
        //    return str;
        //}

        private static ClangSharp.Interop.CXCursor GetTemplateDecl(ClangSharp.Interop.CXCursor cursor)
        {
            switch (cursor.DeclKind)
            {
                case CX_DeclKind.CX_DeclKind_ClassTemplateSpecialization:
                    return GetTemplateDecl(cursor.SpecializedCursorTemplate);
            }

            switch (cursor.Kind)
            {
                case CXCursorKind.CXCursor_ClassTemplate:
                    return cursor;
            }

            return ClangSharp.Interop.CXCursor.Null;
        }


        //private static string GetTypeFullNameLegacy(ClangSharp.Interop.CXCursor cursor)
        //{


        //    string str = cursor.GetFullName();

        //    int numTemplateArguments = cursor.NumTemplateArguments;
        //    if (numTemplateArguments > 0)
        //    {
        //        //      ClangSharp.Interop.CXCursor templateDeclCursor = GetTemplateDecl(cursor);
        //        //      System.Diagnostics.Debug.Assert(templateDeclCursor.IsNull == false, "");

        //        str += "<";
        //        for (uint templateArgumentIndex = 0; templateArgumentIndex < numTemplateArguments; ++templateArgumentIndex)
        //        {

        //            if (templateArgumentIndex > 0)
        //            {
        //                str += $", ";
        //            }

        //            ClangSharp.Interop.CX_TemplateArgument templateArgument = cursor.GetTemplateArgument(templateArgumentIndex);
        //            switch (templateArgument.kind)
        //            {
        //                case CXTemplateArgumentKind.CXTemplateArgumentKind_Type:
        //                    str += GetTypeFullName(templateArgument.AsType);
        //                    break;

        //                case CXTemplateArgumentKind.CXTemplateArgumentKind_Declaration:
        //                    {
        //                        ClangSharp.Interop.CXCursor templateArgumentDecl = templateArgument.AsDecl;

        //                        string s = GetTypeFullName(templateArgumentDecl.Type);
        //                        string firstDeclName;
        //                        int numFields = templateArgumentDecl.Type.Declaration.NumFields;
        //                        if (numFields > 0)
        //                        {
        //                            string declStr = templateArgumentDecl.Spelling.CString;
        //                            {
        //                                int startIndex = declStr.IndexOf('{');
        //                                int endIndex = declStr.IndexOf('}');

        //                                firstDeclName = declStr.Substring(startIndex + 1, endIndex - startIndex - 1);
        //                            }

        //                            {
        //                                int startIndex = 0;
        //                                for (uint fieldIndex = 0; fieldIndex < numFields; ++fieldIndex)
        //                                {
        //                                    ClangSharp.Interop.CXCursor fieldCursor = templateArgumentDecl.Type.Declaration.GetField(fieldIndex);
        //                                    string fieldName = fieldCursor.GetFullName();

        //                                    firstDeclName = firstDeclName.Insert(startIndex, $"static_cast<decltype({fieldName})>(");
        //                                    startIndex = firstDeclName.IndexOf(',', startIndex);
        //                                    if (startIndex >= 0)
        //                                    {
        //                                        firstDeclName = firstDeclName.Insert(startIndex, ")");
        //                                        startIndex = firstDeclName.IndexOf(',', startIndex);
        //                                        startIndex += ",".Length;
        //                                    }
        //                                    else
        //                                    {
        //                                        firstDeclName += ")";
        //                                    }

        //                                }
        //                            }

        //                            str += GetTypeFullName(templateArgument.AsDecl.Type.CanonicalType) + "{" + firstDeclName + "}";
        //                        }


        //                    }
        //                    break;

        //                case CXTemplateArgumentKind.CXTemplateArgumentKind_Integral:
        //                    str += templateArgument.AsIntegral.ToString();
        //                    break;

        //                default:
        //                    System.Diagnostics.Debug.Assert(false, "");
        //                    break;
        //            }

        //        }

        //        str += ">";
        //    }

        //    //  


        //    return str;
        //}

        private static string GetCursorFullNameEx(ClangSharp.Interop.CXCursor cursor)
        {
            string fullName = cursor.Spelling.CString;
            for (ClangSharp.Interop.CXCursor parentCursor = cursor.SemanticParent; parentCursor.IsNull == false; parentCursor = parentCursor.SemanticParent)
            {
                switch (parentCursor.Kind)
                {
                    case CXCursorKind.CXCursor_Namespace:
                        return $"{parentCursor.Spelling.CString}::{fullName}";

                    case CXCursorKind.CXCursor_TranslationUnit:
                        continue;

                    default:

                        break;
                }
            }

            return fullName;
        }

        private void ParseTemplateClass(ClangSharp.Interop.CXCursor cursor)
        {
            Info.TemplateClassUnionInfo CreateClassInfo()
            {
               
                Info.TemplateClassUnionInfo newInfo = new Info.TemplateClassUnionInfo()
                {
                    AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                    CXType = cursor.Type,
                    FullName = cursor.GetFullName(),
                    Namespace = cursor.GetNamespace(),
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    SpecializationsList = [],
                };

                return newInfo;
            }

            void PostProcess(Info.TemplateClassUnionInfo info)
            {
                int numSpecialization = cursor.NumSpecializations;
                string[] nameTable = numSpecialization > 0 ? new string[numSpecialization] : [];
                for (uint i = 0; i < numSpecialization; ++i)
                {
                    CXCursor specializationCursor = cursor.GetSpecialization(i);
                    //  int numArgus = clang.Cursor_getNumTemplateArguments(specializationCursor);

                    ParseCursor(specializationCursor);
                    string fullName = GetTypeFullName(specializationCursor.Type);

                    nameTable[i] = fullName;
                }


                int numDecl = cursor.NumDecls;
                for (uint i = 0; i < numDecl; ++i)
                {
                    CXCursor declCursor = cursor.GetDecl(i);
                    ParseCursor(declCursor);
                }
            }

            uint hash = cursor.Hash;
//            int hash = cursor.Type.CanonicalType.GetHashCode();
            Info.TemplateClassUnionInfo typeInfo = TryCreateBaseInfo(hash, CreateClassInfo, PostProcess);

            Info.IHolder parent = FindParseParent(cursor);
            parent.TypeInfoList.Add(typeInfo);

            if (parent.ToString() == "gggg::Class002<int, float ,double>")
            {
                Util.BreakPoint();
            }
        }

        private void ParseClass(ClangSharp.Interop.CXCursor cursor)
        {
            if(cursor.Spelling.CString.Contains("Application"))
            {
                Util.BreakPoint();
            }

            //  ラムダ式の場合はスキップ
            if (cursor.LambdaCallOperator != ClangSharp.Interop.CXCursor.Null)
            {
                return;
            }

            //  関数内定義の型はスキップ
            if (cursor.ParentFunctionOrMethod.IsNull == false)
            {
                return;
            }

            //  テンプレート引数にラムダ式を含む場合はスキップ
            if (HasLambdaType(cursor.Type.CanonicalType) == true)
            {
                Trace.WarningLine(this, $"ラムダ式を含むテンプレート引数はスキップします {GetTypeFullName(cursor.Type)}");
                return;
            }

            System.Diagnostics.Debug.Assert(cursor.Type.CanonicalType.kind != CXTypeKind.CXType_Invalid);

            if (cursor.IsTemplated == true)
            {
                //System.Diagnostics.Debug.Assert(false, "");
                return;
            }

            if(cursor.Spelling.CString.Contains("CppTest"))
            {
                Util.BreakPoint();
            }

            if(cursor.IsDefinition == false)
            {
                return;
            }

            Info.ClassUnionInfo CreateClassInfo()
            {

                Info.ClassUnionInfo newInfo = new Info.ClassUnionInfo()
                {
                    AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                    CXType = cursor.Type,
                    FullName = GetTypeFullName(cursor.Type),
                    Namespace = cursor.GetNamespace(),
                    AttributeInfoList = GetCustomAttributeList(cursor),
                };

                if (newInfo.FullName.Contains("nox::"))
                {
                    Trace.InfoLine(this, newInfo.FullName);
                }
                return newInfo;
            }

            void PostProcess(Info.ClassUnionInfo info)
            {
                int numDecl = cursor.NumDecls;
                for (uint i = 0; i < numDecl; ++i)
                {
                    CXCursor declCursor = cursor.GetDecl(i);
                    ParseCursor(declCursor);
                }
            }

            uint hash = cursor.Hash;
//            int hash = cursor.Type.CanonicalType.GetHashCode();
            Info.ClassUnionInfo typeInfo = TryCreateBaseInfo(hash, CreateClassInfo, PostProcess);

            Info.IHolder parent = FindParseParent(cursor);
            parent.TypeInfoList.Add(typeInfo);
            return;
        }

        private void ParseVariable(ClangSharp.Interop.CXCursor cursor)
        {
            System.Diagnostics.Debug.Assert(cursor.Type.CanonicalType.kind != CXTypeKind.CXType_Invalid);
            Info.VariableInfo Create()
            {
                Info.VariableInfo variableInfo = new Info.VariableInfo()
                {
                    Name = cursor.Spelling.CString,
                    FullName = cursor.GetFullName()
                };

                return variableInfo;
            }

            void PostProcess(Info.VariableInfo info)
            {
                ParseType(cursor.Type.CanonicalType);

                if(cursor.InitExpr.IsNull == false)
                {
                    ParseCursor(cursor.InitExpr.Definition);
                }
            }

            uint hash = cursor.Hash;
            //int hash = cursor.Type.CanonicalType.GetHashCode();
            Info.VariableInfo variableInfo = TryCreateBaseInfo(hash, Create, PostProcess);

            Info.IHolder parent = FindParseParent(cursor);
            parent.VariableInfoList.Add(variableInfo);
        }

        private void ParseTemplateVariable(ClangSharp.Interop.CXCursor cursor)
        {
            Info.VariableInfo Create()
            {
                int numSpecialization = cursor.NumSpecializations;
                string[] specializationsNameTable = numSpecialization > 0 ? new string[numSpecialization] : [];
                for (uint i = 0; i < numSpecialization; ++i)
                {
                    CXCursor specializationCursor = cursor.GetSpecialization(i);
                    ParseCursor(specializationCursor);
                    string fullName = GetTypeFullName(specializationCursor.Type);

                    specializationsNameTable[i] = fullName;
                }

                Info.TemplateVariableInfo variableInfo = new Info.TemplateVariableInfo()
                {
                    Name = cursor.Spelling.CString,
                    FullName = cursor.GetFullName(),
                    SpecializationsList = specializationsNameTable,
                };

                return variableInfo;
            }

            void PostProcess(Info.VariableInfo info)
            {
                //ParseType(cursor.Type);
            }

            uint hash = cursor.Hash;
            //int hash = cursor.Type.CanonicalType.GetHashCode();
            Info.VariableInfo variableInfo = TryCreateBaseInfo(hash, Create, PostProcess);

            Info.IHolder parent = FindParseParent(cursor);
            parent.VariableInfoList.Add(variableInfo);
        }

        private void ParseTemplateSpecializationVariable(ClangSharp.Interop.CXCursor cursor)
        {
            System.Diagnostics.Debug.Assert(cursor.SpecializedCursorTemplate.IsNull == false, "特殊化元の変数が見つかりません");

            ParseCursor(cursor.SpecializedCursorTemplate.CanonicalCursor);

            Info.TemplateVariableInfo specializedVariableInfo = (Info.TemplateVariableInfo)TypeInfoDict[cursor.SpecializedCursorTemplate.CanonicalCursor.Hash];

            //TODO: 変数テンプレートには未対応
            List<string> specializationsList = (List<string>)specializedVariableInfo.SpecializationsList;
            var result = ParseSpecializationCursor(cursor);
            if(result.hasLambda == true)
            {
                return;
            }
            specializationsList.Add(result.fullName);
        }

        private void ParseFunction(ClangSharp.Interop.CXCursor cursor)
        {
            if(cursor.Spelling.CString.Contains("StaticAssertNoxDeclareReflection"))
            {
                Util.BreakPoint();
            }

            System.Diagnostics.Debug.Assert(cursor.Type.CanonicalType.kind != CXTypeKind.CXType_Invalid);

            var specializationInfo = ParseSpecializationCursor(cursor);
            if(specializationInfo.hasLambda == true)
            {
                return;
            }

            Info.FunctionInfo Create()
            {
             //   ParseType(cursor.Type.CanonicalType);

                int numArguments = cursor.NumArguments;
                uint numDefaultArguments = 0;

                for (uint i = 0; i < numArguments; ++i)
                {
                    CXCursor argCursor = cursor.GetArgument(i);
                    if (argCursor.HasDefaultArg == true)
                    {
                        ++numDefaultArguments;
                    }

               //     ParseType(argCursor.Type.CanonicalType);
                }

            //    ParseType(cursor.ResultType.CanonicalType);

                return new Info.FunctionInfo()
                {
                    Name = cursor.Spelling.CString,
                    FullName = specializationInfo.fullName,
                    NumArguments = numArguments > 0 ? (uint)numArguments : 0,
                    NumDefaultArguments = numDefaultArguments,
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    IsConsteval = cursor.IsConstexpr,
                    IsConstexpr = cursor.IsConstexpr,
                    IsInline = cursor.IsFunctionInlined,
                    IsPureVirtual = cursor.CXXMethod_IsPureVirtual,
                    IsVirtual = cursor.CXXMethod_IsVirtual,
                };
            }

            void PostProcess(Info.FunctionInfo functionInfo)
            {
                ParseType(cursor.Type.CanonicalType);

                int numArguments = cursor.NumArguments;

                for (uint i = 0; i < numArguments; ++i)
                {
                    CXCursor argCursor = cursor.GetArgument(i);
                    ParseType(argCursor.Type.CanonicalType);
                }

                ParseType(cursor.ResultType.CanonicalType);
            }

            uint hash = cursor.Hash;
            //int hash = cursor.Type.CanonicalType.GetHashCode();
            Info.FunctionInfo typeInfo = TryCreateBaseInfo(hash, Create, PostProcess);

            Info.IHolder parent = FindParseParent(cursor);
            parent.FunctionInfoList.Add(typeInfo);
        }

        private (string fullName, bool hasLambda) ParseSpecializationCursor(CXCursor cursor)
        {
            (string fullName, bool hasLambda) result;
            result.fullName = cursor.GetFullName();
            result.hasLambda = false;

            switch(cursor.TemplateSpecializationKind)
            {
                case CX_TemplateSpecializationKind.CX_TSK_ImplicitInstantiation:
                    break;
                default:
                    return result;
            }

            int numTemplateArgument = cursor.SpecializedCursorTemplate.NumTemplateArguments;

            if(numTemplateArgument <= 0)
            {
                return result;
            }

            result.fullName += "<";
            for(uint i = 0; i < numTemplateArgument; ++i)
            {
                if(i > 0)
                {
                    result.fullName += ",";
                }

                ClangSharp.Interop.CX_TemplateArgument templateArgument = cursor.GetTemplateArgument(i);
                switch(templateArgument.kind)
                {
                    case CXTemplateArgumentKind.CXTemplateArgumentKind_Type:
                        if (HasLambdaType(templateArgument.AsType.CanonicalType) == true)
                        {
                            result.hasLambda = true;
                        }
                        result.fullName += templateArgument.AsType.CanonicalType.Spelling.CString;
                        break;

                    case CXTemplateArgumentKind.CXTemplateArgumentKind_Integral:
                        result.fullName += templateArgument.AsIntegral.ToString();
                        break;

                    case CXTemplateArgumentKind.CXTemplateArgumentKind_Declaration:
                        const string begin = "<template param ";
                        const string end = ">";
                        ClangSharp.Interop.CXCursor templateArgumentCursor = templateArgument.AsDecl;
                        ClangSharp.Interop.CXType canonicalType = GetTypeWithQualifiersRemoved(templateArgumentCursor.Type.CanonicalType);
                        if(HasLambdaType(canonicalType) == true)
                        {
                            result.hasLambda = true;
                        }
                        string tmp = templateArgumentCursor.Spelling.CString;
                        int beginIndex = tmp.IndexOf(begin);
                        int endIndex = tmp.LastIndexOf(end);

                        string removeTmpStr = tmp.Substring(beginIndex + begin.Length, endIndex - (beginIndex + begin.Length));

                        string indexTypeNameBegin = removeTmpStr.Substring(removeTmpStr.IndexOf('{'));

                        result.fullName += canonicalType.Spelling.CString + indexTypeNameBegin;
                        break;

                    default:
                        continue;
//                        System.Diagnostics.Debug.Assert(false);
 //                       break;
                }
            }

            result.fullName += ">";
            return result;    
        }

        private void ParseTemplateFunction(ClangSharp.Interop.CXCursor cursor)
        {
            System.Diagnostics.Debug.Assert(cursor.Type.CanonicalType.kind != CXTypeKind.CXType_Invalid);

            Info.TemplateFunctionInfo Create()
            {
                int numSpecialization = cursor.NumSpecializations;
                List<string> specializationsNameList = new List<string>();
                for (uint i = 0; i < numSpecialization; ++i)
                {
                    CXCursor specializationCursor = cursor.GetSpecialization(i);
                //    ParseCursor(specializationCursor);
                    var result = ParseSpecializationCursor(specializationCursor);
                    if(result.hasLambda == true)
                    {
                        continue;
                    }
                    specializationsNameList.Add(result.fullName);
                }

                int numArguments = cursor.NumArguments;
                uint numDefaultArguments = 0;

                for (uint i = 0; i < numArguments; ++i)
                {
                    CXCursor argCursor = cursor.GetArgument(i);
                    if (argCursor.HasDefaultArg == true)
                    {
                        ++numDefaultArguments;
                    }

                //    ParseType(argCursor.Type);
                }

                return new Info.TemplateFunctionInfo()
                {
                    Name = cursor.Spelling.CString,
                    FullName = cursor.GetFullName(),
                    NumArguments = numArguments > 0 ? (uint)numArguments : 0,
                    NumDefaultArguments = numDefaultArguments,
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    IsConsteval = cursor.IsConstexpr,
                    IsConstexpr = cursor.IsConstexpr,
                    IsInline = cursor.IsFunctionInlined,
                    IsPureVirtual = cursor.CXXMethod_IsPureVirtual,
                    IsVirtual = cursor.CXXMethod_IsVirtual,
                    SpecializationsList = [],
                };
            }

            void PostProcess(Info.TemplateFunctionInfo functionInfo)
            {
                int numSpecialization = cursor.NumSpecializations;
                List<string> specializationsNameList = new List<string>();
                for (uint i = 0; i < numSpecialization; ++i)
                {
                    CXCursor specializationCursor = cursor.GetSpecialization(i);
                    ParseCursor(specializationCursor);
                }

                int numArguments = cursor.NumArguments;

                for (uint i = 0; i < numArguments; ++i)
                {
                    CXCursor argCursor = cursor.GetArgument(i);
                    ParseType(argCursor.Type);
                }
            }

            uint hash = cursor.Hash;
            //int hash = cursor.Type.CanonicalType.GetHashCode();
            Info.FunctionInfo typeInfo = TryCreateBaseInfo(hash, Create, PostProcess);

            Info.IHolder parent = FindParseParent(cursor);
            parent.FunctionInfoList.Add(typeInfo);
        }


        private void ParseCursor(ClangSharp.Interop.CXCursor cursor)
        {
            if(cursor.Spelling.CString.Contains("Application") == true)
            {
                Util.BreakPoint();
            }

            switch (cursor.Kind)
            {
                case CXCursorKind.CXCursor_Namespace:
                    ParseNamespace(cursor);
                    break;

                case CXCursorKind.CXCursor_ClassDecl:
                case CXCursorKind.CXCursor_StructDecl:
                    ParseClass(cursor);
                    break;

                case CXCursorKind.CXCursor_ClassTemplatePartialSpecialization:
                case CXCursorKind.CXCursor_ClassTemplate:
                    ParseTemplateClass(cursor.CanonicalCursor);
                    break;

                case CXCursorKind.CXCursor_VarDecl:
                case CXCursorKind.CXCursor_FieldDecl:
                    ParseVariable(cursor);
                    break;

                case CXCursorKind.CXCursor_FirstDecl:
                    //  テンプレート変数の特殊化
                    switch (cursor.DeclKind)
                    {
                        case CX_DeclKind.CX_DeclKind_VarTemplate:
                       //     ParseTemplateVariable(cursor);
                            break;

                        case CX_DeclKind.CX_DeclKind_FirstVarTemplateSpecialization:
                      //      ParseTemplateSpecializationVariable(cursor);
                            break;
                    }
                    break;

                case CXCursorKind.CXCursor_Constructor:
                case CXCursorKind.CXCursor_Destructor:
                    break;

                case CXCursorKind.CXCursor_FunctionDecl:
                case CXCursorKind.CXCursor_CXXMethod:
                    ParseFunction(cursor);
                    break;

                case CXCursorKind.CXCursor_FunctionTemplate:
                    ParseTemplateFunction(cursor);
                    break;

                case CXCursorKind.CXCursor_EnumDecl:
                    ParseEnum(cursor);
                    break;

                case CXCursorKind.CXCursor_TypeAliasTemplateDecl:
                    ParseTemplateTypedef(cursor.TemplatedDecl);
                    break;

                case CXCursorKind.CXCursor_TypedefDecl:
                case CXCursorKind.CXCursor_TypeAliasDecl:
                    ParseTypedef(cursor);
                    break;

                case CXCursorKind.CXCursor_CXXAccessSpecifier:
                case CXCursorKind.CXCursor_UsingDirective:
                    break;

                

                default:

                    break;
            }
        }
        private void ParseRoot(ClangSharp.Interop.CXCursor cursor)
        {
            uint hash = cursor.Hash;
            RootDeclHolder = new Info.DeclHolder()
            {
                Namespace = string.Empty,
            };
            TypeInfoDict.Add(hash, RootDeclHolder);

            int numDecl = cursor.NumDecls;
            for (uint i = 0; i < numDecl; ++i)
            {
                CXCursor declCursor = cursor.GetDecl(i);
                ParseCursor(declCursor);
            }
        }

        private void ParseNamespace(ClangSharp.Interop.CXCursor cursor)
        {
            uint hash = cursor.Hash;

            if(TypeInfoDict.ContainsKey(hash) == true)
            {
                return;
            }

            Info.DeclHolder declHolder;
            {
                if (TypeInfoDict.TryGetValue(hash, out Info.IBaseInfo? declHolderOld) == true)
                {
                    declHolder = (Info.DeclHolder)declHolderOld;
                }
                else
                {
                    declHolder = new Info.DeclHolder()
                    {
                        Namespace = cursor.GetNamespace()
                    };
                }
            }

            //{
            //    if (_DeclHolderDict.TryGetValue(namespaceStr, out Info.DeclHolder? declHolderOld) == true)
            //    {
            //        declHolder = declHolderOld;
            //    }
            //    else
            //    {
            //        declHolder = new Info.DeclHolder()
            //        {
            //            Namespace = cursor.GetNamespace()
            //        };
            //        _DeclHolderDict.Add(namespaceStr, declHolder);
            //    }
            //}

            TypeInfoDict.Add(hash, declHolder);

            int numDecl = cursor.NumDecls;
            for (uint i = 0; i < numDecl; ++i)
            {
                CXCursor declCursor = cursor.GetDecl(i);
                ParseCursor(declCursor);
            }

            FindParseParent<Info.DeclHolder>(cursor).DeclHolderList.Add(declHolder);
        }
        #endregion

#if false
        private enum Kind
        {
            Namespace,
            Class,
            Enum,
            Function,
            Variable,
            Field,
            Method,
            Type,
            TemplateType,
            TemplateParamType,
            TemplateVariable,
            Lambda,
            Return,
            UnaryExpr,
        }

        private abstract class BaseData
        {

        }

        private class Data : BaseData
        {
            private Data? _Parent = null;

            public required Kind Kind { get; init; }

            public required Data? Parent
            {
                get => _Parent;
                init
                {
                    _Parent = value;
                    if(_Parent == null)
                    {
                        return;
                    }

                    if(_Parent.Child == null)
                    {
                        _Parent.Child = this;
                    }
                    else
                    {
                        for(Data next = _Parent.Child; ; next = next.Next)
                        {
                            if(next.Next == null)
                            {
                                next.Next = this;
                                break;
                            }
                        }
                    }
                }
            }

            public Data? Child { get; set; } = null;

            public Data? Next { get; set; } = null;

            public Data? Pointee { get; set; } = null;

            public IReadOnlyList<Data> GetChildren()
            {
                List<Data> paramList = new List<Data>();
                for(Data? next = Child; next != null; next = next.Next)
                {
                    paramList.Add(next);
                }

                return paramList;
            }
        }

        private class TypeData : Data
        {
            public required string FullName { get; init; }
        }

        private class TemplateTypeParamData : TypeData
        {
            public required int TemplateDepth { get; init; }
            public required int TemplateListIndex { get; init; }
        }

        private class TypeRefData : Data
        {
            public uint CurosrHash { get; init; } = 0;
            public CXTypeKind CursorKind { get; init; }

            public bool IsBuiltin => CurosrHash == 0;
        }

        private class NamespaceData : Data
        {
            public required string Namespace { get; init; }
        }
        private class UnaryExpressionData : Data
        {
            public required string ExpressionStr { get; init; }
        }

        private ClangSharp.Interop.CXChildVisitResult VisitChild_ClassTemplate(ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXCursor parent, void* client_data)
        {
            Data parentParam = System.Runtime.CompilerServices.Unsafe.AsRef<Data>(client_data);
            switch (cursor.Kind)
            {
                case CXCursorKind.CXCursor_TemplateTypeParameter:

                    break;
            }

            return CXChildVisitResult.CXChildVisit_Continue;
        }

        private ClangSharp.Interop.CXChildVisitResult VisitChild_Function(ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXCursor parent, void* client_data)
        {
            Data parentParam = System.Runtime.CompilerServices.Unsafe.AsRef<Data>(client_data);

            Data param;
            void CursorVisitChild() => cursor.VisitChildren(VisitChild_Function, (CXClientData)System.Runtime.CompilerServices.Unsafe.AsPointer(ref param));

            switch (cursor.Kind)
            {
                case CXCursorKind.CXCursor_TypeRef:
                    param = new TypeData()
                    {
                        Kind = Kind.Type,
                        Parent = parentParam,
                        FullName = cursor.Spelling.CString
                    };
                    return CXChildVisitResult.CXChildVisit_Break;

                case CXCursorKind.CXCursor_CompoundStmt:
                    return CXChildVisitResult.CXChildVisit_Recurse;

                case CXCursorKind.CXCursor_ReturnStmt:
                    param = new Data()
                    {
                        Kind = Kind.Return,
                        Parent = parentParam
                    };

                    CursorVisitChild();
                    return CXChildVisitResult.CXChildVisit_Recurse;

                case CXCursorKind.CXCursor_UnaryExpr:
                    param = new UnaryExpressionData()
                    {
                        Kind = Kind.UnaryExpr,
                        Parent = parentParam,
                        ExpressionStr = cursor.Spelling.CString
                    };
                    CursorVisitChild();
                    return CXChildVisitResult.CXChildVisit_Recurse;
            }

            return CXChildVisitResult.CXChildVisit_Continue;
        }

        private ClangSharp.Interop.CXChildVisitResult VisitChild_Type()
        {
            return CXChildVisitResult.CXChildVisit_Continue;
        }

        private ClangSharp.Interop.CXChildVisitResult VisitChild(ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXCursor parent, void* client_data)
        {
            Data parentParam = System.Runtime.CompilerServices.Unsafe.AsRef<Data>(client_data);

            Data param;
            void CursorVisitChild(ClangSharp.Interop.CXCursorVisitor visitor)
            {
                cursor.VisitChildren(visitor, (CXClientData)System.Runtime.CompilerServices.Unsafe.AsPointer(ref param));
            };

            void CursorVisitChildDefault() => CursorVisitChild(VisitChild);

            switch (cursor.Kind)
            {
                case CXCursorKind.CXCursor_TypeRef:
                    param = parentParam;
                    CursorVisitChildDefault();
                    return CXChildVisitResult.CXChildVisit_Recurse;

                case CXCursorKind.CXCursor_TypedefDecl:
                    break;

                case CXCursorKind.CXCursor_TemplateRef:
                    return CXChildVisitResult.CXChildVisit_Recurse;

                case CXCursorKind.CXCursor_TypeAliasDecl:
                    CXType typedefDeclUnderyingType = cursor.TypedefDeclUnderlyingType;
                    
                    //  buildin
                    switch(typedefDeclUnderyingType.TypeClass)
                    {
                        case CX_TypeClass.CX_TypeClass_Builtin:
                            param = new TypeRefData()
                            {
                                Kind = Kind.Type,
                                Parent = parentParam,
                                CursorKind = typedefDeclUnderyingType.kind
                            };
                            break;
                    }
                    if(typedefDeclUnderyingType.TypeClass == CX_TypeClass.CX_TypeClass_Builtin)
                    {
                        
                    }
                    else
                    {
                        param = new TypeRefData()
                        {
                            Kind = Kind.Type,
                            Parent = parentParam,
                            CurosrHash = typedefDeclUnderyingType.Declaration.Hash
                        };
                    }

                 //   CursorVisitChildDefault();
                    return CXChildVisitResult.CXChildVisit_Recurse;

                case CXCursorKind.CXCursor_CompoundStmt:
                    return CXChildVisitResult.CXChildVisit_Recurse;

                case CXCursorKind.CXCursor_Namespace:
                    {
                        param = new NamespaceData()
                        {
                            Kind = Kind.Namespace,
                            Parent = parentParam,

                            Namespace = cursor.Spelling.CString
                        };

                        CursorVisitChildDefault();
                    }
                    break;

                case CXCursorKind.CXCursor_ClassDecl:

                    break;

                case CXCursorKind.CXCursor_TypeAliasTemplateDecl:
                    param = new TypeData()
                    {
                        Kind = Kind.TemplateType,
                        Parent = parentParam,
                        FullName = cursor.Spelling.CString
                    };

                    CursorVisitChildDefault();
                    return CXChildVisitResult.CXChildVisit_Recurse;

                case CXCursorKind.CXCursor_LambdaExpr:
                    {
                        param = new Data()
                        {
                            Kind = Kind.Lambda,
                            Parent = parentParam,
                        };

                        CursorVisitChild(VisitChild_Function);
                    }
                    break;

                case CXCursorKind.CXCursor_TemplateTypeParameter:
               //     System.Diagnostics.Debug.Assert(parentParam != null);
                        param = new Data()
                        {
                            Kind = Kind.TemplateParamType,
                            Parent = parentParam,
                        };
                        CursorVisitChildDefault();
                    return CXChildVisitResult.CXChildVisit_Recurse;

                case CXCursorKind.CXCursor_ClassTemplate:
                    {
                     
                    }
                    break;
            }

            return CXChildVisitResult.CXChildVisit_Continue;

            //string scope = string.Empty;
            //for(ClangSharp.Interop.CXCursor tempParent = parent; tempParent != ClangSharp.Interop.CXCursor.Null; tempParent = tempParent.LexicalParent)
            //{
            //    scope += "\t";
            //}

            //if (cursor.Kind == CXCursorKind.CXCursor_ClassDecl)
            //{
            //    Trace.Info(this, $"{scope}{cursor.Spelling.CString}");
            //}
            //cursor.VisitChildren(VisitChild, default);
            //return CXChildVisitResult.CXChildVisit_Continue;

            if (
                cursor.Spelling.CString.Contains("intValue") ||
                cursor.Spelling.CString.Contains("app::Int") 
                )
            {
            }

            if (
                cursor.Spelling.CString.Contains("asBody") ||
                parent.Spelling.CString.Contains("asValue") 
                )
            {
            }

            if (cursor.IsTemplated)
            {
                System.Diagnostics.Debug.Assert(cursor.NumTemplateParameterLists <= 1, $"未対応の識別値です: {cursor.Spelling.CString}");
            }

            switch (cursor.Kind)
            {
                //  namespace
                case ClangSharp.Interop.CXCursorKind.CXCursor_Namespace:
                    //ParseNamespace(cursor);
                    cursor.VisitChildren(VisitChild, default);
                    break;

                //  class, struct, union
                case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_StructDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_UnionDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_CXXBaseSpecifier:
                case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplate:

                    //    ParseClassDecl(cursor);
                    cursor.VisitChildren(VisitChild, default);
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
                    ParseFunction(cursor);
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
                    Trace.InfoLine(this, $"Kind:{cursor.Kind.ToString()}");
                //    System.Diagnostics.Debug.Assert(false, $"未対応の識別値です {cursor.Type.CanonicalType.KindSpelling.CString}");
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
                    Trace.InfoLine(this, "");
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_CompoundStmt:
                case ClangSharp.Interop.CXCursorKind.CXCursor_DeclStmt:
                    cursor.VisitChildren(VisitChild, default);
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_EnumDecl:
                    ParseEnum(cursor);
                    break;

                case CXCursorKind.CXCursor_TemplateTypeParameter:
                case CXCursorKind.CXCursor_LambdaExpr:
                case CXCursorKind.CXCursor_ReturnStmt:
                case CXCursorKind.CXCursor_IntegerLiteral:
                    var pp = cursor.GetNumTemplateParameters(0);



                    if (cursor.Type.CanonicalType.Spelling.CString.Contains("TestClass00"))
                    {
                        Util.BreakPoint();
                    }
                    cursor.VisitChildren(VisitChild, default);
                    
                    break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_BinaryOperator:
                case ClangSharp.Interop.CXCursorKind.CXCursor_UnaryOperator:
                default:
                    cursor.VisitChildren(VisitChild, default);
                    break;

            }

            return ClangSharp.Interop.CXChildVisitResult.CXChildVisit_Continue;
        }
#endif

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
                        Trace.WarningLine(this, $".vcxprojファイルが複数見つかりました　最初に見つかったファイルをmodule名として扱います\n{vcxProjectFileList}");
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
                        Trace.InfoLine(this, templateArg.AsType.Spelling.CString);
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
            if (_IgnoreNamespaceList.Count > 0)
            {
                if (_IgnoreNamespaceList.Contains(namespaceStr) == true)
                {
                    return false;
                }
            }

            if (_EnableRootNamespaceList.Count <= 0)
            {
                return true;
            }

            List<string> namespaceStrList = cursor.GetNamespaceList();
            if (namespaceStrList.Count <= 0)
            {
                return false;
            }

            if (_EnableRootNamespaceList.Contains(namespaceStrList[0]) == true)
            {
                return true;
            }

            return false;
        }

        private bool IsParseNamespace()
        {
            if (_IgnoreNamespaceList.Count > 0)
            {
                if (_IgnoreNamespaceList.Contains(GetFullNamespaceScope()) == true)
                {
                    return false;
                }
            }

            if (_EnableRootNamespaceList.Count > 0)
            {
                if (_EnableRootNamespaceList.Contains(GetFullNamespaceScope()) == false)
                {
                    return false;
                }
            }

            return true;
        }
#if false
        #region パース関係

        private Info.ClassInfoOld CreateClassInfo(ClangSharp.Interop.CXCursor cursor)
        {
            //  継承型
            string[] baseClassFullNameList = new string[int.Max(0, cursor.NumBases)];

            for (uint i = 0; i < baseClassFullNameList.Length; ++i)
            {
                ClangSharp.Interop.CXCursor baseCursor = cursor.GetBase(i);
                baseClassFullNameList[i] = baseCursor.Type.GetCanonicalTypeFullName();
            }

            //  
            Info.ClassInfoOld newClassInfo = new Info.ClassInfoOld()
            {
                Namespace = cursor.GetNamespace(),
                Module = CreateModule(cursor),
                CXCursor = cursor,
                Name = cursor.Type.GetCanonicalTypeName(),
                FullName = cursor.Type.GetCanonicalTypeFullName(),
                AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),

                IsTypedef = cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasDecl,
                AttributeInfoList = GetAttributeInfoList(cursor),
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

                            case CXTemplateArgumentKind.CXTemplateArgumentKind_Expression:
                                break;

                            default:
                           //     System.Diagnostics.Debug.Assert(false, $"未対応のTemplateArgumentKind:{nativeArgumentInfo.kind.ToString()}");
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

                Info.ClassInfoOld newClassInfo = CreateClassInfo(cursor);

                ClassInfoStack.Push(newClassInfo);
                cursor.VisitChildren(VisitChild, default);
                ClassInfoStack.Pop();
            }
        }

        private void ParseFunction(ClangSharp.Interop.CXCursor cursor)
        {
            //  所属クラスがリフレクション対象でなければ、AccessLevel::Publicのみ収集する
            if (ClassInfoStack.EmptyStack == false)
            {
                if (ClassInfoStack.Current.IsReflection == false && cursor.CXXAccessSpecifier.GetAccessLevel() != AccessLevel.Public)
                {
                    return;
                }
            }

            //  属性の取得
           

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
                FullName  = cursor.GetObjectFullName(),
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
                AttributeInfoList = GetAttributeInfoList(cursor)
            };

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

            Info.FieldInfo field = new Info.FieldInfo()
            {
                Module = CreateModule(cursor),
                Name = cursor.Spelling.CString,
                FullName = cursor.GetObjectFullName(),
                RuntimeType = cursor.CreateRuntimeType(cursor.Type),
                AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                IsConstexpr = cursor.IsConstexpr,
                IsTemplate = cursor.IsTemplated,
                AttributeInfoList = GetAttributeInfoList(cursor),
            };

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
                Info.ClassInfoOld classInfo = CreateClassInfo(cursor.Type.Declaration);
                ClassInfoStack.Add(classInfo);
            }
        }

        private void ParseEnum(ClangSharp.Interop.CXCursor cursor)
        {
            if (cursor.IsDefinition == false)
            {
                return;
            }

            Info.EnumInfo.EnumVariable[] enumVariableList = new Info.EnumInfo.EnumVariable[int.Max(0, cursor.NumEnumerators)];
            for(uint i = 0; i < enumVariableList.Length; i++)
            {
                ClangSharp.Interop.CXCursor enumCursor = cursor.GetEnumerator(i);
                enumVariableList[i] = new Info.EnumInfo.EnumVariable
                {
                    Name = enumCursor.Spelling.CString,
                    AttributeInfoList = GetAttributeInfoList(enumCursor)
                };
            }

            Info.EnumInfo enumInfo = new Info.EnumInfo
            {
                Name = cursor.Spelling.CString,
                FullName = cursor.GetObjectFullName(),
                Namespace = cursor.GetNamespace(),
                Module = CreateModule(cursor),
                VariableList = enumVariableList,
                AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),

                AttributeInfoList = GetAttributeInfoList(cursor),
            };

            EnumInfoStack.Push(enumInfo);
        }
        #endregion
#endif
        #endregion
    }
}
