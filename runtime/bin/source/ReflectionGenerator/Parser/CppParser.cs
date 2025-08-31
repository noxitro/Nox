using ClangSharp.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
#if false
namespace ReflectionGenerator.Parser
{
    /// <summary>
    /// Cpp解析
    /// </summary>
    public unsafe class CppParser
    {
        #region 公開定義
       
        /// <summary>
        /// 制御
        /// </summary>
        public readonly struct SetupDesc
        {
            /// <summary>
            /// 解析対象ファイルパス
            /// </summary>
            public required string SourceFilePath { get; init; }

            /// <summary>
            /// 
            /// </summary>
            public required string SolutionPath { get; init; }

            /// <summary>
            /// 
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
            /// c++バージョン
            /// clang用の定義で入っている(-std=c++2bなど)
            /// </summary>
            public required string CppVersion { get; init; }

            /// <summary>
            /// 最適化オプション
            /// </summary>
            public required string Optimization { get; init; }

			/// <summary>
			/// プリプロセッサマクロ定義群
			/// ;区切りで入っている
			/// </summary>
			public required string PreprocessorMacro { get; init; }

			/// <summary>
			/// 解析対象外のroot namespaceリスト
			/// </summary>
			public required IReadOnlyList<string> IgnoreNamespaceList { get; init; }

            /// <summary>
            /// 解析対象のnamespace
            /// nullの場合、全てが対象
            /// </summary>
            public required IReadOnlyList<string> EnableRootNamespaceList { get; init; }

			/// <summary>
			/// 追加インクルードディレクトリ
			/// </summary>
			public required string AdditionalIncludeDirectories { get; init; }

            /// <summary>
            /// 追加オプション
            /// </summary>
            public required string AdditionalOptions { get; init; }

			/// <summary>
			/// 実行時型情報を使用するか
			/// </summary>
			public required bool UseRtti { get; init; }

			/// <summary>
			/// モジュールごとのヘッダーファイルリスト
			/// </summary>
			public required IReadOnlyDictionary<string, IReadOnlyList<string>> IncludeHeaderListWithArtifact { get; init; }
		}
        #endregion

        #region 非公開フィールド
        private Info.NamespaceInfoStack _NamespaceInfoStack = new Info.NamespaceInfoStack();

        /// <summary>
        /// 無視するroot namespace list
        /// </summary>
        private IReadOnlyList<string> _IgnoreNamespaceList = [];


        /// <summary>
        /// 解析対象のroot namespace list
        /// </summary>
        private IReadOnlyList<string> _EnableRootNamespaceList = [];

        /// <summary>
        /// 
        /// </summary>
        private string _ProjectRootDirectory = string.Empty;

        /// <summary>
        /// テンプレートクラスを解析するかどうか
        /// </summary>
        private const bool _EnabledTemplateParse = false;

        private readonly Dictionary<int, string> _TypeNameDict = new Dictionary<int, string>();

        private readonly Dictionary<int, Info.TypeData> _TypeDataDict = new Dictionary<int, Info.TypeData>();

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

        private ClangSharp.Interop.CXIndex _RootCXIndex = default;
		private ClangSharp.Interop.CXTranslationUnit _RootTransUnit = default;
        #endregion

        #region 公開プロパティ
		/// <summary>
		/// モジュール名リスト
		/// </summary>
		public List<string> ModuleNameList { get; } = new List<string>();

        public Info.NamespaceDeclInfo? RootDeclHolder { get; private set; } = null;

        private Dictionary<long, Info.IBaseInfo> TypeInfoDict { get; } = new Dictionary<long, Info.IBaseInfo>();
        
        /// <summary>
        /// 型情報リスト
        /// key:    モジュール名
        /// value:  型情報リスト
        /// </summary>
        public Dictionary<string, List<Info.NamespaceDeclInfo>> TypeInfoListWithModuleNameDict { get; } = new Dictionary<string, List<Info.NamespaceDeclInfo>>();
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
		private static bool parseBuildOptions(out List<string> outClangCommandLineList, string msbuildBinPath, string solutionPath, string projectPath, string configuration, string platform, string sourceFilePath)
        {
            outClangCommandLineList = new List<string>();

            if (System.IO.File.Exists(sourceFilePath) == false)
            {
                Trace.ErrorLine(null, $"解析対象のソースファイルが存在しません:{sourceFilePath}");
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

			//string args = 
			//$"/p:Configuration={configuration};Platform={platform} {solutionPath} /property:GenerateFullPaths=true /v:n /t:clean /p:SelectedFiles=\"{sourceFilePath}\" /p:PreprocessToFile=true /p:PreprocessOutput={outputFilePath} /verbosity:diag";

			// vcvarsall.bat を実行して環境変数を設定

			if (System.IO.File.Exists(solutionPath.Replace("\"", "")) == false)
            {
                return false;
            }

            if(System.IO.File.Exists(msbuildPath) == false)
            {
                return false;
            }

            using System.Diagnostics.Process process = new System.Diagnostics.Process();

            {
                System.Diagnostics.ProcessStartInfo startInfo = new System.Diagnostics.ProcessStartInfo();
                startInfo.FileName = msbuildPath;
                startInfo.Arguments = args;
                startInfo.Verb = "RunAs";
                startInfo.UseShellExecute = false;
                startInfo.RedirectStandardOutput = true;
                startInfo.StandardOutputEncoding = System.Text.Encoding.UTF8;
                startInfo.CreateNoWindow = true;
                startInfo.UseShellExecute = false;

                startInfo.RedirectStandardError = true;


				process.StartInfo = startInfo;

			}

			System.Text.StringBuilder stdout = new System.Text.StringBuilder();
			System.Text.StringBuilder stderr = new System.Text.StringBuilder();

			System.Diagnostics.Stopwatch stopwatchProcess = new System.Diagnostics.Stopwatch();
            stopwatchProcess.Start();
            process.Start();

			process.OutputDataReceived += (sender, e) => { if (e.Data != null) { stdout.AppendLine(e.Data); } }; // 標準出力に書き込まれた文字列を取り出す
			process.ErrorDataReceived += (sender, e) => { if (e.Data != null) { stderr.AppendLine(e.Data); } }; // 標準エラー出力に書き込まれた文字列を取り出す


			//process.OutputDataReceived += new System.Diagnostics.DataReceivedEventHandler((sender, e) => {
			//    if (!string.IsNullOrEmpty(e.Data))
			//    {
			//        Console.WriteLine(e.Data);
			//    }
			//});
			process.BeginOutputReadLine();
            //	process.BeginErrorReadLine();


            process.WaitForExit();
            {
           //    return false;
            }
            stopwatchProcess.Stop();
            Trace.InfoLine(null, $"MSBuildにかかった時間:{stopwatchProcess.ElapsedMilliseconds.ToString()}msec");

			if (process.ExitCode != 0)
			{
				Trace.ErrorLine(null, $"MSBuildエラー ExitCode:{process.ExitCode}\n{stderr.ToString()}");
				return false;
			}
			//    string allStr = process.StandardOutput.ReadToEnd();

			//  CL.exeに渡している引数情報を取得する
			string clArgs = string.Empty;
            {
                string rawStr = stdout.ToString();

				ReadOnlySpan<string> lines = rawStr.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None);

                foreach (string line in lines)
                {
                    int index = line.IndexOf("ClCompile:");
                    if (index < 0)
                    {
                        return false;
                    }

                    clArgs = line.Substring(index + "ClCompile:".Length);
                }
			}
				//foreach(var s in stdout.get)
				//{
				//    string? lineStr = process.StandardOutput.ReadLine();
				//    if(lineStr == null)
				//    {
				//        break;
				//    }

				//    int index = lineStr.IndexOf("ClCompile:");
				//    if (index < 0)
				//    {
				//        continue;
				//    }

				//    //  CL.exeの引数情報を取得
				//    clArgs = process.StandardOutput.ReadLine() ?? string.Empty;
				//    //                clArgs = lineStr.Substring(index + "CL.exe".Length);

				//    break;
				//}


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
        public static string? CreateParseSourceFile(string solutionPath, IReadOnlyDictionary<string, IReadOnlyList<string>> includeHeaderListWithArtifactDict)
        {
            string parseSourceFilePath = System.IO.Path.GetFullPath($"{System.IO.Path.GetTempPath()}/tmpParseSource.cpp");

            if(System.IO.File.Exists(parseSourceFilePath) == true)
            {
                System.IO.File.Delete(parseSourceFilePath);
            }

            //  ソリューションファイルからプロジェクトファイルパスリストを取得する
        
            using (System.IO.StreamWriter streamWriter = new System.IO.StreamWriter(parseSourceFilePath, false, System.Text.Encoding.UTF8))
            {
                foreach ((string artifactName, IReadOnlyList<string> headerFileList) in includeHeaderListWithArtifactDict)
                {
                    //IReadOnlyList<string> headerFiles = ExtractVCXProjectFile.ExtractHeaderFiles(projectFilePath);
                    foreach (ReadOnlySpan<char> headerFile in headerFileList)
                    {
                        streamWriter.WriteLine($"#include\t\"{headerFile}\"");
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
        public bool Parse(in SetupDesc setupParam)
        {
			_ProjectRootDirectory = System.IO.Path.GetDirectoryName(setupParam.SolutionPath) ?? string.Empty;

            {
                //  パースするソースファイルを作成
#if false
                string? parseSourceFilePath = CreateParseSourceFile(setupParam.SolutionPath, setupParam.IncludeHeaderListWithArtifact);
                if(parseSourceFilePath == null)
                {
                    Trace.Error(this, "パース用ソースファイルの作成に失敗しました");
                    return false;
                }
#endif
                string parseSourceFilePath = setupParam.SourceFilePath;

				//  引数
				List<string> parseCommandLineList = new List<string>();

				//  解析時にのみ有効にするマクロ
				parseCommandLineList.Add($"-D {Define.RUNTIME_REFLECTION_GENERATOR_DEFINE}");

                //  カスタムタスクで解析した情報をセット
				parseCommandLineList.Add(setupParam.CppVersion);

				//  ビルド構成 プラットフォーム

				//  最適化オプション
				//  msvcの定義からclangの定義に変換
                string optimizationOption = setupParam.Optimization switch
				{
					"Disabled" => "-O0",
					"MinSpace" => "-O1",
					"MaxSpeed" => "-O2",
					"Full" => "-O3",
					_ => string.Empty,
				};

				if (optimizationOption != string.Empty)
				{
					parseCommandLineList.Add(optimizationOption);
				}
                else
                {
					Trace.ErrorLine(null, $"不明な最適化オプションです:{setupParam.Optimization}");
				}

				//  define
				ReadOnlySpan<string> macros = setupParam.PreprocessorMacro.Split(';', StringSplitOptions.RemoveEmptyEntries);
				foreach (string macro in macros)
				{
					parseCommandLineList.Add($"-D {macro}");
				}

				//  追加インクルードディレクトリ
				ReadOnlySpan<string> additionalIncludeDirectories = setupParam.AdditionalIncludeDirectories.Split(';', StringSplitOptions.RemoveEmptyEntries);
				foreach (string additionalIncludeDirectory in additionalIncludeDirectories)
				{
					parseCommandLineList.Add($"-I {additionalIncludeDirectory}");
				}

				//  end

				//Example.Exe(parseSourceFilePath, parseCommandLineList.ToArray());

				//  不明な属性を無視しない
				_RootCXIndex = ClangSharp.Interop.CXIndex.Create(true, true);
                ClangSharp.Interop.CXErrorCode cxErrorCode;
                //  コンパイル
                //   for (int i = 0; i < 100; ++i)
                {
                    System.Diagnostics.Stopwatch stopwatchClangCompile = new System.Diagnostics.Stopwatch();
                    stopwatchClangCompile.Start();
                    cxErrorCode = ClangSharp.Interop.CXTranslationUnit.TryParse(
					 _RootCXIndex,
                     parseSourceFilePath,
                     parseCommandLineList.ToArray(),
                     default,
					// 関数の中身を解析しないことで、高速化を試みる
					ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_SkipFunctionBodies |
                    ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_PrecompiledPreamble | 
                    ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_CacheCompletionResults |
                    ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_KeepGoing |
                    ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_IgnoreNonErrorsFromIncludedFiles
					,
                  //  ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_None,
                     out _RootTransUnit
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
                    for (uint i = 0; i < _RootTransUnit.NumDiagnostics; ++i)
                    {
                        ClangSharp.Interop.CXDiagnostic diagnostic = _RootTransUnit.GetDiagnostic(i);
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
               
                //p.DumpTrace();
				//ParseRoot(_RootTransUnit.Cursor);
				//ParseClassCursor(_RootTransUnit.Cursor, default);

			}

            return true;
        }

        public void Dispose()
        {
            if (_RootTransUnit != default)
            {
                _RootTransUnit.Dispose();
                _RootTransUnit = default;
			}

			if (_RootCXIndex != default)
            {
                _RootCXIndex.Dispose();
                _RootCXIndex = default;
			}
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
                            attrList[i] = new Info.StandardAttribute()
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
                        attrList[i] = new Info.StandardAttribute()
                        {
                            AttrKind = attrCursor.AttrKind,
                        };
                        break;
                }
            }

            return attrList;
        }

        private void TryCreateBaseInfo<T>(long hash, Func<T> action, Action<T>? post = null) where T : Info.IBaseInfo
        {
            if (TypeInfoDict.TryGetValue(hash, out Info.IBaseInfo? baseInfo) == true)
            {
//                (T)baseInfo;
                return;
            }

            if(TypeInfoDict.ContainsKey(hash) == true)
            {
                System.Diagnostics.Debug.Assert(false);
            }

            T v = action();
            TypeInfoDict.Add(hash, v);
            post?.Invoke(v);
     //       return v;
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
                        IsUnsigned = enumCursor.IsUnsigned,
						Integer64 = integer64,
                        AttributeInfoList = GetCustomAttributeList(cursor),
                        Name = enumCursor.Name.CString
                    };
                }

                Info.EnumInfo enumInfo = new Info.EnumInfo(enumVariableList)
                {
                    Name = cursor.Spelling.CString,
                    AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                    Namespace = cursor.GetNamespace(),
                    FullName = cursor.GetFullName(),
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    CXType = cursor.Type,
                    Module = CreateModule(cursor)
                };

                return enumInfo;
            }

            void PostProcess(Info.EnumInfo info)
            {
                Info.IHolder parent = FindParseParent(cursor);
                parent.EnumInfoList.Add(info);
            }

            uint hash = cursor.Hash;
            TryCreateBaseInfo(hash, Create, PostProcess);
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
            if(cxType.GetHashCode() != cxType.CanonicalType.GetHashCode())
            {
                Util.BreakPoint();
            }

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
                                AttributeInfoList = [],
                                Module = Info.ArtifactData.Invalid,
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
                                AttributeInfoList = [],
                                Module = Info.ArtifactData.Invalid,
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
                                AttributeInfoList = [],
                                Module = Info.ArtifactData.Invalid,
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
                    Module = CreateModule(cursor),
                    IsAttribute = false,
                    IsReflectionObject = false,
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

            //    Info.IHolder parent = FindParseParent(cursor);
           //     parent.ClassInfoList.Add(info);
            }

            uint hash = cursor.Hash;
//            int hash = cursor.Type.CanonicalType.GetHashCode();
            TryCreateBaseInfo(hash, CreateClassInfo, PostProcess);
        }

        private void ParseClass(ClangSharp.Interop.CXCursor cursor)
        {
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

            if(cursor.IsDefinition == false)
            {
                return;
            }

            Info.ClassInfo CreateClassInfo()
            {
                bool isAttributeClass = false;
                bool isReflectionObject = false;

              

				Info.ClassInfo newInfo = new Info.ClassInfo()
                {
                    AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                    CXType = cursor.Type,
                    Name = cursor.Spelling.CString,
					FullName = GetTypeFullName(cursor.Type),
                    Namespace = cursor.GetNamespace(),
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    Module = CreateModule(cursor),
                    IsAttribute = isAttributeClass,
//                    IsReflectionObject = isReflectionObject,
				};

                return newInfo;
            }

            void PostProcess(Info.ClassInfo info)
            {
                int numDecl = cursor.NumDecls;
                for (uint i = 0; i < numDecl; ++i)
                {
                    CXCursor declCursor = cursor.GetDecl(i);
                    ParseCursor(declCursor);
                }

                Info.IHolder parent = FindParseParent(cursor);
                parent.ClassInfoList.Add(info);

                if(info.FullName == "nox::reflection::ReflectionObject")
                {
                    info.IsReflectionObject = true;
				}

				if (info.Name == "FunctionInfo")
				{
					Util.BreakPoint();
				}

				if (info.Name == "InvokeArgument")
				{
					Util.BreakPoint();
				}

				Info.ClassInfo? parentClassInfo = parent as Info.ClassInfo;
                if(parentClassInfo != null)
                {
                    info.BaseTypeInfoList.Add(parentClassInfo);

                    info.IsReflectionObject = parentClassInfo.IsReflectionObject;
					//  Attributeクラスでなければ、親クラスとして設定 
					if (parentClassInfo.IsReflectionObject == true)
                    {
                        info.ParentTypeInfo = parentClassInfo;
                    }
				}
			}

            uint hash = cursor.Hash;
//            int hash = cursor.Type.CanonicalType.GetHashCode();
            TryCreateBaseInfo(hash, CreateClassInfo, PostProcess);
            return;
        }

        private void ParseFriend(ClangSharp.Interop.CXCursor cursor)
        {
            string fullName = cursor.Type.GetCanonicalTypeFullName();
            string fullName1 = cursor.FriendDecl.Type.GetCanonicalTypeFullName();
            string fullName2 = cursor.Spelling.CString;
            string fullName3 = cursor.FriendDecl.Spelling.CString;

            if (fullName != string.Empty)
            {
            //    Trace.InfoLine(this, fullName);
            }

            if (fullName1 != string.Empty)
            {
          //      Trace.InfoLine(this, fullName1);
            }

            if (fullName2 != string.Empty)
            {
           //     Trace.InfoLine(this, fullName2);
            }

            if (fullName3 != string.Empty)
            {
           //     Trace.InfoLine(this, fullName3);
            }


            if (fullName.Contains("nox::reflection::ReflectionGeneratedHolder") == true)
            {
                Info.ClassInfo parent = FindParseParent<Info.ClassInfo>(cursor);
                parent.IsPrivateReflection = true;
            }
        }

        private void ParseVariable(ClangSharp.Interop.CXCursor cursor)
        {
            System.Diagnostics.Debug.Assert(cursor.Type.CanonicalType.kind != CXTypeKind.CXType_Invalid);
            Info.VariableInfo Create()
            {
                //  ここで変数の型を解析
                ParseType(cursor.Type.CanonicalType);

                AccessLevel accessLevel;
                bool isStatic = cursor.IsStatic == true || cursor.Kind == ClangSharp.Interop.CXCursorKind.CXCursor_VarDecl;
                if(isStatic == true)
                {
                    //  グローバル変数ならPublic扱い
                    accessLevel = AccessLevel.Public;
                }
                else
                {
                    accessLevel = cursor.CXXAccessSpecifier.GetAccessLevel();
				}

                if (cursor.Spelling.CString.Contains("display_name_"))
                {
                    Trace.Info(this, "");
                }
				if (cursor.Spelling.CString.Contains("prev"))
				{
					Trace.Info(this, "");
				}
				TypeInfoDict.TryGetValue(cursor.Type.CanonicalType.Declaration.Hash, out Info.IBaseInfo? outTypeInfo);

				Info.VariableInfo variableInfo = new Info.VariableInfo()
                {
                    VariableTypeInfo = outTypeInfo as Info.ClassInfo,
                    InitTypeData = cursor.Type.GetTypeData(),
                    Name = cursor.Spelling.CString,
                    FullName = cursor.GetFullName(),
                    Namespace = cursor.GetNamespace(),
                    Module = CreateModule(cursor),
                    AccessLevel = accessLevel,
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    Offset = cursor.OffsetOfField,
                    BitWith = cursor.FieldDeclBitWidth,
                    IsConstexpr = cursor.IsConstexpr,   //  MEMO:   グローバル変数は何故かConstexpr判定がうまくされない
                    IsStatic = isStatic,
                };
                return variableInfo;
            }

            void PostProcess(Info.VariableInfo info)
            {
              //  ParseType(cursor.Type.CanonicalType);

                if(cursor.InitExpr.IsNull == false)
                {
                    ParseCursor(cursor.InitExpr.Definition);
                }

                Info.IHolder parent = FindParseParent(cursor);
                parent.VariableInfoList.Add(info);
            }

            uint hash = cursor.Hash;
            //int hash = cursor.Type.CanonicalType.GetHashCode();
            TryCreateBaseInfo(hash, Create, PostProcess);

       
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
					InitTypeData = cursor.Type.GetTypeData(),
                    Name = cursor.Spelling.CString,
                    FullName = cursor.GetFullName(),
                    Namespace = cursor.GetNamespace(),
                    SpecializationsList = specializationsNameTable,
                    Module = CreateModule(cursor),
                    AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    BitWith = cursor.FieldDeclBitWidth,
                    Offset = cursor.OffsetOfField,
                    IsConstexpr = cursor.IsConstexpr,
                    IsStatic = cursor.IsStatic,
                    VariableTypeInfo = null,
                };

                return variableInfo;
            }

            void PostProcess(Info.VariableInfo info)
            {
                Info.IHolder parent = FindParseParent(cursor);
                parent.VariableInfoList.Add(info);
            }

            uint hash = cursor.Hash;
            //int hash = cursor.Type.CanonicalType.GetHashCode();
            TryCreateBaseInfo(hash, Create, PostProcess);
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

        private static string GetCanonicalFunctionTypeName(ClangSharp.Interop.CXCursor cursor)
        {
            if(cursor.IsGlobal == true)
            {
                return cursor.Type.GetCanonicalTypeFullName();
            }

			// メンバ関数の場合
			string functionType = cursor.Type.CanonicalType.Spelling.CString;
			string returnType = cursor.ResultType.CanonicalType.Spelling.CString;
			string classType = cursor.ThisType.PointeeType.UnqualifiedType.GetCanonicalTypeFullName();

			// 戻り値型の後にクラス型とメンバポインタ記法を挿入
			int returnTypeIndex = functionType.IndexOf(returnType);
			if (returnTypeIndex >= 0)
			{
				int insertPosition = returnTypeIndex + returnType.Length;
				return functionType.Insert(insertPosition, $" ({classType}::*)");
			}

			return functionType;
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

            if(cursor.IsDeleted == true)
            {
                Trace.WarningLine(this, $"削除された関数はスキップします {cursor.GetFullName()}");
                return;
			}

            if(cursor.Spelling.CString.Contains("IsConst"))
            {
                Util.BreakPoint();
			}

			Info.FunctionInfo Create()
            {
                int numArguments = cursor.NumArguments;
                uint numDefaultArguments = 0;

                Info.FunctionInfo.ArgumentInfo[] argumentInfoList = new Info.FunctionInfo.ArgumentInfo[numArguments];
				for (uint i = 0; i < numArguments; ++i)
                {
                    CXCursor argCursor = cursor.GetArgument(i);
                    if (argCursor.HasDefaultArg == true)
                    {
                        ++numDefaultArguments;
                    }

                    ParseType(argCursor.Type.CanonicalType);

                    argumentInfoList[i] = new Info.FunctionInfo.ArgumentInfo()
                    {
                        IsDefault = argCursor.HasDefaultArg,
                        Name = argCursor.Spelling.CString,
                        TypeFullName = argCursor.Type.CanonicalType.GetCanonicalTypeFullName(),
                    };
				}

                if(specializationInfo.fullName == "nox::attr::Attribute::GetUnderlyingType")
                {
                    Util.BreakPoint();
                }

                return new Info.FunctionInfo()
                {
                    Name = cursor.Spelling.CString,
                    FullName = specializationInfo.fullName,
                    Namespace = specializationInfo.fullName,
                    FunctionTypeFullName = GetCanonicalFunctionTypeName(cursor),
                    NumArguments = numArguments > 0 ? (uint)numArguments : 0,
                    NumDefaultArguments = numDefaultArguments,
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    IsConsteval = false,
                    IsConstexpr = cursor.IsConstexpr,
                    IsInline = cursor.IsFunctionInlined,
                    IsPureVirtual = cursor.CXXMethod_IsPureVirtual,
                    IsVirtual = cursor.CXXMethod_IsVirtual,
                    OperatorKind = cursor.OverloadedOperatorKind,
                    Module = CreateModule(cursor),
					IsDefaultConstructor = cursor.CXXConstructor_IsDefaultConstructor,
					IsCopyConstructor = cursor.CXXConstructor_IsCopyConstructor,
					IsMoveConstructor = cursor.CXXConstructor_IsMoveConstructor,
					IsDestructor = cursor.Destructor != ClangSharp.Interop.CXCursor.Null,
                    AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                    ArgumentInfoList = argumentInfoList,
					IsNoReturn = cursor.IsNoReturn,
                    IsNoexcept = 
                        cursor.Type.ExceptionSpecificationType == ClangSharp.Interop.CXCursor_ExceptionSpecificationKind.CXCursor_ExceptionSpecificationKind_BasicNoexcept ||
                        cursor.Type.ExceptionSpecificationType == ClangSharp.Interop.CXCursor_ExceptionSpecificationKind.CXCursor_ExceptionSpecificationKind_ComputedNoexcept
				};
            }

            void PostProcess(Info.FunctionInfo info)
            {
                ParseType(cursor.Type.CanonicalType);

                int numArguments = cursor.NumArguments;

                for (uint i = 0; i < numArguments; ++i)
                {
                    CXCursor argCursor = cursor.GetArgument(i);
                    ParseType(argCursor.Type.CanonicalType);
                }

                ParseType(cursor.ResultType.CanonicalType);

                Info.IHolder parent = FindParseParent(cursor);
                parent.FunctionInfoList.Add(info);
            }

            uint hash = cursor.Hash;
            //int hash = cursor.Type.CanonicalType.GetHashCode();
            TryCreateBaseInfo(hash, Create, PostProcess);

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

                int numArguments = Math.Max(0, cursor.NumArguments);
                uint numDefaultArguments = 0;

                Info.FunctionInfo.ArgumentInfo[] argumentInfoList = new Info.FunctionInfo.ArgumentInfo[numArguments];
				for (uint i = 0; i < numArguments; ++i)
                {
                    CXCursor argCursor = cursor.GetArgument(i);
                    if (argCursor.HasDefaultArg == true)
                    {
                        ++numDefaultArguments;
                    }

                    ParseType(argCursor.Type);
                }

				
				return new Info.TemplateFunctionInfo()
                {
                    Name = cursor.Spelling.CString,
                    FullName = cursor.GetFullName(),
                    Namespace = cursor.GetNamespace(),
                    FunctionTypeFullName = cursor.Type.CanonicalType.GetCanonicalTypeFullName(),
                    NumArguments = numArguments > 0 ? (uint)numArguments : 0,
                    NumDefaultArguments = numDefaultArguments,
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    IsConsteval = cursor.IsConstexpr,
                    IsConstexpr = cursor.IsConstexpr,
                    IsInline = cursor.IsFunctionInlined,
                    IsPureVirtual = cursor.CXXMethod_IsPureVirtual,
                    IsVirtual = cursor.CXXMethod_IsVirtual,
                    SpecializationsList = [],
                    OperatorKind = cursor.OverloadedOperatorKind,
                    Module = CreateModule(cursor),
                    IsDefaultConstructor = cursor.CXXConstructor_IsDefaultConstructor,
                    IsCopyConstructor = cursor.CXXConstructor_IsCopyConstructor,
                    IsMoveConstructor = cursor.CXXConstructor_IsMoveConstructor,
                    IsDestructor = cursor.Destructor != ClangSharp.Interop.CXCursor.Null,
					AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
                    ArgumentInfoList = argumentInfoList,
                    IsNoReturn = cursor.IsNoReturn,
                    IsNoexcept = false
				};
            }

            void PostProcess(Info.TemplateFunctionInfo info)
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

                Info.IHolder parent = FindParseParent(cursor);
                parent.FunctionInfoList.Add(info);
            }

            uint hash = cursor.Hash;
            //int hash = cursor.Type.CanonicalType.GetHashCode();
            TryCreateBaseInfo(hash, Create, PostProcess);
        }


        private void ParseClassCursor(ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXCursor parentCursor = default)
        {
            switch (cursor.kind)
            {
                case CXCursorKind.CXCursor_TranslationUnit:
                    
					cursor.VisitChildren((child, parent, data) =>
					{
						ParseClassCursor(child);
						return CXChildVisitResult.CXChildVisit_Continue;
					}, default);
					break;

                case CXCursorKind.CXCursor_Namespace:
                    Trace.InfoLine(null, $"Namespace: {cursor.GetFullName()}, {cursor.Type.kind}");

					cursor.VisitChildren((child, parent, data) =>
					{
						ParseClassCursor(child);
						return CXChildVisitResult.CXChildVisit_Continue;
					}, default);
                    break;

                case CXCursorKind.CXCursor_ClassDecl:
                case CXCursorKind.CXCursor_StructDecl:
                case CXCursorKind.CXCursor_UnionDecl:
				case CXCursorKind.CXCursor_TypedefDecl:
				case CXCursorKind.CXCursor_TypeAliasDecl:
				case CXCursorKind.CXCursor_TypeAliasTemplateDecl:
					if (!cursor.IsDefinition)
						return;

                    if (cursor.IsDefined == true)
                    {
                        return;
                    }

					Trace.InfoLine(null, $"Class: {cursor.GetFullName()}, {cursor.Type.kind}");
					cursor.VisitChildren((child, parent, data) =>
					{
						ParseClassCursor(child);
						return CXChildVisitResult.CXChildVisit_Continue;
					}, default);
					break;

                case CXCursorKind.CXCursor_EnumDecl:
                    Trace.InfoLine(null, $"Enum: {cursor.GetFullName()}, {cursor.Type.kind}");
					cursor.VisitChildren((child, parent, data) =>
					{
						ParseClassCursor(child);
						return CXChildVisitResult.CXChildVisit_Continue;
					}, default);
					break;

                default:
                    break;
            }
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
                case CXCursorKind.CXCursor_UnionDecl:
                    ParseClass(cursor);
                    break;

                case CXCursorKind.CXCursor_FriendDecl:
                    ParseFriend(cursor);
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
              //      ParseTemplateFunction(cursor);
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
           
            Info.NamespaceDeclInfo Create()
            {
                return RootDeclHolder = new Info.NamespaceDeclInfo()
                {
                    Namespace = string.Empty,
                    AttributeInfoList = [],
                    Module = CreateModule(cursor),
                };
            }

            void PostProcess(Info.NamespaceDeclInfo info)
            {
                int numDecl = cursor.NumDecls;
                for (uint i = 0; i < numDecl; ++i)
                {
                    CXCursor declCursor = cursor.GetDecl(i);
                    ParseCursor(declCursor);
                }

                //  module
                if (TypeInfoListWithModuleNameDict.TryGetValue(info.Module.ModuleName, out List<Info.NamespaceDeclInfo>? infoList) == false || infoList == null)
                {
                    TypeInfoListWithModuleNameDict.Add(info.Module.ModuleName, infoList = new List<Info.NamespaceDeclInfo>());
                }
                infoList.Add(info);
            }
            TryCreateBaseInfo(hash, Create, PostProcess);
        }

        private void ParseNamespace(ClangSharp.Interop.CXCursor cursor)
        {
            uint hash = cursor.Hash;
            Info.NamespaceDeclInfo Create()
            {
                return new Info.NamespaceDeclInfo()
                {
                    Namespace = cursor.GetNamespace(),
                    AttributeInfoList = GetCustomAttributeList(cursor),
                    Module = CreateModule(cursor),
                };
            }

            void PostProcess(Info.NamespaceDeclInfo info)
            {
                int numDecl = cursor.NumDecls;
                for (uint i = 0; i < numDecl; ++i)
                {
                    CXCursor declCursor = cursor.GetDecl(i);
                    ParseCursor(declCursor);
                }
                FindParseParent<Info.NamespaceDeclInfo>(cursor).DeclHolderList.Add(info);

                //  module
                if (TypeInfoListWithModuleNameDict.TryGetValue(info.Module.ModuleName, out List<Info.NamespaceDeclInfo>? infoList) == false || infoList == null)
                {
                    TypeInfoListWithModuleNameDict.Add(info.Module.ModuleName, infoList = new List<Info.NamespaceDeclInfo>());
                }
                infoList.Add(info);
            }

            TryCreateBaseInfo(hash, Create, PostProcess);
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
        private Info.ArtifactData CreateModule(ClangSharp.Interop.CXCursor cursor)
        {
            

            //  ソリューションディレクトまで辿って、.vcxprojを探す
            //  なければ、unnamedとして扱う
            cursor.Location.GetFileLocation(out ClangSharp.Interop.CXFile outFile, out uint outLine, out uint outColumn, out uint outOffset);
            if(outFile.Handle == 0)
            {
                return new Info.ArtifactData() { ModuleName = Define.UNKNOWN_MODULE_NAME };
            }
            string? path = System.IO.Path.GetDirectoryName(outFile.Name.CString);

            string moduleName = Define.UNKNOWN_MODULE_NAME;

            //  ルートディレクトリ内か？
            if (path == null || path.StartsWith(_ProjectRootDirectory) == false)
            {
                return new Info.ArtifactData() { ModuleName = moduleName };
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

            Info.ArtifactData module = new Info.ArtifactData() { ModuleName = moduleName };

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

        #region テスト
		/// <summary>
		/// ClangSharp 全型情報収集器
		/// </summary>
		public class ComprehensiveTypeCollector : IDisposable
		{
            #region 型情報定義

			/// <summary>
			/// 収集された型情報の基底クラス
			/// </summary>
			public abstract class CollectedTypeInfo
			{
				public required string Name { get; init; }
				public required string FullName { get; init; }
				public required string Namespace { get; init; }
				public required string SourceFile { get; init; }
				public required uint Line { get; init; }
				public required uint Column { get; init; }
				public required CXCursor OriginalCursor { get; init; }
				public required CXType OriginalType { get; init; }
			}

			/// <summary>
			/// 基本型情報（int, float等）
			/// </summary>
			public class BuiltinTypeInfo : CollectedTypeInfo
			{
				public required CXTypeKind TypeKind { get; init; }
				public required bool IsSigned { get; init; }
				public required uint SizeBytes { get; init; }
				public required uint AlignBytes { get; init; }
			}

			/// <summary>
			/// レコード型情報（class, struct, union）
			/// </summary>
			public class RecordTypeInfo : CollectedTypeInfo
			{
				public required RecordKind Kind { get; init; }
				public required AccessLevel AccessLevel { get; init; }
				public required IReadOnlyList<FieldInfo> Fields { get; init; }
				public required IReadOnlyList<MethodInfo> Methods { get; init; }
				public required IReadOnlyList<string> BaseClasses { get; init; }
				public required IReadOnlyList<RecordTypeInfo> NestedTypes { get; init; }
				public required bool IsTemplated { get; init; }
				public required bool IsComplete { get; init; }
				public long SizeBytes { get; init; } = -1;
				public uint AlignBytes { get; init; } = 0;

				public enum RecordKind { Class, Struct, Union }
			}

			/// <summary>
			/// 列挙型情報
			/// </summary>
			public class EnumTypeInfo : CollectedTypeInfo
			{
				public required bool IsScoped { get; init; }
				public required string UnderlyingType { get; init; }
				public required IReadOnlyList<EnumValue> Values { get; init; }
				public required AccessLevel AccessLevel { get; init; }

				public readonly struct EnumValue
				{
					public required string Name { get; init; }
					public required long Value { get; init; }
					public required bool IsUnsigned { get; init; }
				}
			}

			/// <summary>
			/// テンプレート型情報
			/// </summary>
			public class TemplateTypeInfo : CollectedTypeInfo
			{
				public required IReadOnlyList<TemplateParameter> Parameters { get; init; }
				public required IReadOnlyList<string> Specializations { get; init; }
				public required TemplateKind Kind { get; init; }

				public enum TemplateKind { Class, Function, Variable, TypeAlias }

				public readonly struct TemplateParameter
				{
					public required string Name { get; init; }
					public required ParameterKind Kind { get; init; }
					public string DefaultValue { get; init; } = "";

					public enum ParameterKind { Type, NonType, Template }

                    public TemplateParameter() { }
				}
			}

			/// <summary>
			/// 関数型情報
			/// </summary>
			public class FunctionTypeInfo : CollectedTypeInfo
			{
				public required string ReturnType { get; init; }
				public required IReadOnlyList<ParameterInfo> Parameters { get; init; }
				public required bool IsVariadic { get; init; }
				public required CallingConvention CallingConv { get; init; }
				public required bool IsNoExcept { get; init; }

				public readonly struct ParameterInfo
				{
					public required string Name { get; init; }
					public required string Type { get; init; }
					public required bool HasDefault { get; init; }
				}

				public enum CallingConvention { C, StdCall, FastCall, ThisCall, VectorCall }
			}

			/// <summary>
			/// ポインタ・参照型情報
			/// </summary>
			public class PointerTypeInfo : CollectedTypeInfo
			{
				public required string PointeeType { get; init; }
				public required PointerKind Kind { get; init; }
				public required IReadOnlyList<string> Qualifiers { get; init; }

				public enum PointerKind { Pointer, LValueReference, RValueReference }
			}

			/// <summary>
			/// 配列型情報
			/// </summary>
			public class ArrayTypeInfo : CollectedTypeInfo
			{
				public required string ElementType { get; init; }
				public required ArrayKind Kind { get; init; }
				public long Size { get; init; } = -1; // -1 = 不明・可変長

				public enum ArrayKind { ConstantSize, IncompleteSize, VariableSize }
			}

			/// <summary>
			/// typedef/type alias情報
			/// </summary>
			public class TypedefInfo : CollectedTypeInfo
			{
				public required string UnderlyingType { get; init; }
				public required bool IsTemplateAlias { get; init; }
			}

			/// <summary>
			/// フィールド情報
			/// </summary>
			public readonly struct FieldInfo
			{
				public required string Name { get; init; }
				public required string Type { get; init; }
				public required AccessLevel Access { get; init; }
				public required bool IsStatic { get; init; }
				public required bool IsConst { get; init; }
				public required bool IsMutable { get; init; }
				public required long OffsetBits { get; init; }
				public required uint BitFieldWidth { get; init; }
			}

			/// <summary>
			/// メソッド情報
			/// </summary>
			public readonly struct MethodInfo
			{
				public required string Name { get; init; }
				public required string Signature { get; init; }
				public required AccessLevel Access { get; init; }
				public required bool IsStatic { get; init; }
				public required bool IsVirtual { get; init; }
				public required bool IsPure { get; init; }
				public required bool IsConst { get; init; }
				public required bool IsOverride { get; init; }
				public required bool IsNoExcept { get; init; }
			}

			/// <summary>
			/// アクセスレベル
			/// </summary>
			public enum AccessLevel { Public, Protected, Private }

			/// <summary>
			/// 収集結果
			/// </summary>
			public readonly struct CollectionResult
			{
				public required IReadOnlyList<BuiltinTypeInfo> BuiltinTypes { get; init; }
				public required IReadOnlyList<RecordTypeInfo> RecordTypes { get; init; }
				public required IReadOnlyList<EnumTypeInfo> EnumTypes { get; init; }
				public required IReadOnlyList<TemplateTypeInfo> TemplateTypes { get; init; }
				public required IReadOnlyList<FunctionTypeInfo> FunctionTypes { get; init; }
				public required IReadOnlyList<PointerTypeInfo> PointerTypes { get; init; }
				public required IReadOnlyList<ArrayTypeInfo> ArrayTypes { get; init; }
				public required IReadOnlyList<TypedefInfo> TypedefTypes { get; init; }

				public int TotalTypeCount =>
					BuiltinTypes.Count + RecordTypes.Count + EnumTypes.Count +
					TemplateTypes.Count + FunctionTypes.Count + PointerTypes.Count +
					ArrayTypes.Count + TypedefTypes.Count;
			}

            #endregion

            #region フィールド

			private CXIndex _index;
			private CXTranslationUnit _translationUnit;

			private readonly List<BuiltinTypeInfo> _builtinTypes = new();
			private readonly List<RecordTypeInfo> _recordTypes = new();
			private readonly List<EnumTypeInfo> _enumTypes = new();
			private readonly List<TemplateTypeInfo> _templateTypes = new();
			private readonly List<FunctionTypeInfo> _functionTypes = new();
			private readonly List<PointerTypeInfo> _pointerTypes = new();
			private readonly List<ArrayTypeInfo> _arrayTypes = new();
			private readonly List<TypedefInfo> _typedefTypes = new();

			private readonly HashSet<uint> _processedCursors = new();
			private readonly Dictionary<string, CollectedTypeInfo> _typeCache = new();

            #endregion

            #region 公開メソッド

			/// <summary>
			/// ソースファイルから全型情報を収集
			/// </summary>
			/// <param name="sourceFile">C++ソースファイルパス</param>
			/// <param name="compilerArgs">コンパイラ引数</param>
			/// <returns>収集結果</returns>
			public CollectionResult CollectAllTypes(string sourceFile, ReadOnlySpan<string> compilerArgs)
			{
				// ClangSharpの初期化
				_index = CXIndex.Create();

				var errorCode = CXTranslationUnit.TryParse(
					_index,
					sourceFile,
					compilerArgs,
					ReadOnlySpan<CXUnsavedFile>.Empty,
					CXTranslationUnit_Flags.CXTranslationUnit_DetailedPreprocessingRecord,
					out _translationUnit);

				if (errorCode != CXErrorCode.CXError_Success)
				{
					throw new InvalidOperationException($"Failed to parse file: {sourceFile}, Error: {errorCode}");
				}

				// 診断情報の確認
				CheckDiagnostics();

				// コレクションをクリア
				ClearCollections();

				// ルートから再帰的に解析
				var rootCursor = _translationUnit.Cursor;
				VisitCursor(rootCursor);

				// 結果を返す
				return new CollectionResult
				{
					BuiltinTypes = _builtinTypes.AsReadOnly(),
					RecordTypes = _recordTypes.AsReadOnly(),
					EnumTypes = _enumTypes.AsReadOnly(),
					TemplateTypes = _templateTypes.AsReadOnly(),
					FunctionTypes = _functionTypes.AsReadOnly(),
					PointerTypes = _pointerTypes.AsReadOnly(),
					ArrayTypes = _arrayTypes.AsReadOnly(),
					TypedefTypes = _typedefTypes.AsReadOnly()
				};
			}

			/// <summary>
			/// 型情報を検索
			/// </summary>
			/// <param name="typeName">型名</param>
			/// <returns>見つかった型情報</returns>
			public CollectedTypeInfo? FindType(string typeName)
			{
				return _typeCache.TryGetValue(typeName, out var type) ? type : null;
			}

			/// <summary>
			/// 収集統計を表示
			/// </summary>
			public void PrintStatistics()
			{
				Console.WriteLine("=== 型情報収集統計 ===");
				Console.WriteLine($"基本型: {_builtinTypes.Count}");
				Console.WriteLine($"レコード型 (class/struct/union): {_recordTypes.Count}");
				Console.WriteLine($"列挙型: {_enumTypes.Count}");
				Console.WriteLine($"テンプレート型: {_templateTypes.Count}");
				Console.WriteLine($"関数型: {_functionTypes.Count}");
				Console.WriteLine($"ポインタ/参照型: {_pointerTypes.Count}");
				Console.WriteLine($"配列型: {_arrayTypes.Count}");
				Console.WriteLine($"Typedef/エイリアス: {_typedefTypes.Count}");
				Console.WriteLine($"総計: {_builtinTypes.Count + _recordTypes.Count + _enumTypes.Count + _templateTypes.Count + _functionTypes.Count + _pointerTypes.Count + _arrayTypes.Count + _typedefTypes.Count}");
			}

			public void Dispose()
			{
				if (_translationUnit.Handle != IntPtr.Zero)
				{
					_translationUnit.Dispose();
				}
				if (_index.Handle != IntPtr.Zero)
				{
					_index.Dispose();
				}
			}

            #endregion

            #region 内部メソッド

			/// <summary>
			/// 診断情報をチェック
			/// </summary>
			private void CheckDiagnostics()
			{
				for (uint i = 0; i < _translationUnit.NumDiagnostics; i++)
				{
					var diagnostic = _translationUnit.GetDiagnostic(i);
					var severity = diagnostic.Severity;
					var message = diagnostic.Spelling.CString;

					switch (severity)
					{
						case CXDiagnosticSeverity.CXDiagnostic_Error:
						case CXDiagnosticSeverity.CXDiagnostic_Fatal:
							Console.Error.WriteLine($"Error: {message}");
							break;
						case CXDiagnosticSeverity.CXDiagnostic_Warning:
							Console.WriteLine($"Warning: {message}");
							break;
					}
				}
			}

			/// <summary>
			/// コレクションをクリア
			/// </summary>
			private void ClearCollections()
			{
				_builtinTypes.Clear();
				_recordTypes.Clear();
				_enumTypes.Clear();
				_templateTypes.Clear();
				_functionTypes.Clear();
				_pointerTypes.Clear();
				_arrayTypes.Clear();
				_typedefTypes.Clear();
				_processedCursors.Clear();
				_typeCache.Clear();
			}

			/// <summary>
			/// カーソルを訪問
			/// </summary>
			/// <param name="cursor">カーソル</param>
			private void VisitCursor(CXCursor cursor)
			{
				// 既に処理済みかチェック
				var hash = cursor.Hash;
				if (_processedCursors.Contains(hash))
					return;

				_processedCursors.Add(hash);

				// カーソルの種類に応じて処理
				switch (cursor.Kind)
				{
					case CXCursorKind.CXCursor_ClassDecl:
					case CXCursorKind.CXCursor_StructDecl:
					case CXCursorKind.CXCursor_UnionDecl:
						ProcessRecordType(cursor);
						break;

					case CXCursorKind.CXCursor_EnumDecl:
						ProcessEnumType(cursor);
						break;

					case CXCursorKind.CXCursor_ClassTemplate:
					case CXCursorKind.CXCursor_FunctionTemplate:
						ProcessTemplateType(cursor);
						break;

					case CXCursorKind.CXCursor_TypedefDecl:
					case CXCursorKind.CXCursor_TypeAliasDecl:
						ProcessTypedefType(cursor);
						break;

					case CXCursorKind.CXCursor_FunctionDecl:
					case CXCursorKind.CXCursor_CXXMethod:
						ProcessFunctionType(cursor);
						break;

					case CXCursorKind.CXCursor_VarDecl:
					case CXCursorKind.CXCursor_FieldDecl:
						ProcessVariableType(cursor);
						break;
				}

				// 子要素を再帰的に処理
				cursor.VisitChildren((child, parent, data) =>
				{
					VisitCursor(child);
					return CXChildVisitResult.CXChildVisit_Continue;
				}, default);

				// 型情報も解析
				AnalyzeType(cursor.Type);
			}

			/// <summary>
			/// 型を解析
			/// </summary>
			/// <param name="type">型</param>
			private void AnalyzeType(CXType type)
			{
				if (type.kind == CXTypeKind.CXType_Invalid)
					return;

				var canonicalType = type.CanonicalType;
				var typeName = canonicalType.Spelling.CString;

				// 既に処理済みかチェック
				if (_typeCache.ContainsKey(typeName))
					return;

				switch (canonicalType.kind)
				{
					case CXTypeKind.CXType_Void:
					case CXTypeKind.CXType_Bool:
					case CXTypeKind.CXType_Char_U:
					case CXTypeKind.CXType_UChar:
					case CXTypeKind.CXType_Char16:
					case CXTypeKind.CXType_Char32:
					case CXTypeKind.CXType_UShort:
					case CXTypeKind.CXType_UInt:
					case CXTypeKind.CXType_ULong:
					case CXTypeKind.CXType_ULongLong:
					case CXTypeKind.CXType_Char_S:
					case CXTypeKind.CXType_SChar:
					case CXTypeKind.CXType_WChar:
					case CXTypeKind.CXType_Short:
					case CXTypeKind.CXType_Int:
					case CXTypeKind.CXType_Long:
					case CXTypeKind.CXType_LongLong:
					case CXTypeKind.CXType_Float:
					case CXTypeKind.CXType_Double:
					case CXTypeKind.CXType_LongDouble:
						ProcessBuiltinType(canonicalType);
						break;

					case CXTypeKind.CXType_Pointer:
					case CXTypeKind.CXType_LValueReference:
					case CXTypeKind.CXType_RValueReference:
						ProcessPointerType(canonicalType);
						break;

					case CXTypeKind.CXType_ConstantArray:
					case CXTypeKind.CXType_IncompleteArray:
					case CXTypeKind.CXType_DependentSizedArray:
						ProcessArrayType(canonicalType);
						break;

					case CXTypeKind.CXType_FunctionProto:
					case CXTypeKind.CXType_FunctionNoProto:
						ProcessFunctionTypeFromType(canonicalType);
						break;

					case CXTypeKind.CXType_Record:
						if (!canonicalType.Declaration.IsNull)
							ProcessRecordType(canonicalType.Declaration);
						break;

					case CXTypeKind.CXType_Enum:
						if (!canonicalType.Declaration.IsNull)
							ProcessEnumType(canonicalType.Declaration);
						break;
				}

				// ポインター先の型も解析
				if (canonicalType.kind == CXTypeKind.CXType_Pointer ||
					canonicalType.kind == CXTypeKind.CXType_LValueReference ||
					canonicalType.kind == CXTypeKind.CXType_RValueReference)
				{
					AnalyzeType(canonicalType.PointeeType);
				}

				// 配列要素型も解析
				if (canonicalType.kind == CXTypeKind.CXType_ConstantArray ||
					canonicalType.kind == CXTypeKind.CXType_IncompleteArray ||
					canonicalType.kind == CXTypeKind.CXType_DependentSizedArray)
				{
					AnalyzeType(canonicalType.ElementType);
				}
			}

			/// <summary>
			/// 基本型を処理
			/// </summary>
			private void ProcessBuiltinType(CXType type)
			{
				var info = new BuiltinTypeInfo
				{
					Name = type.Spelling.CString,
					FullName = type.Spelling.CString,
					Namespace = "",
					SourceFile = "<built-in>",
					Line = 0,
					Column = 0,
					OriginalCursor = default,
					OriginalType = type,
					TypeKind = type.kind,
					IsSigned = !type.Spelling.CString.Contains("unsigned"),
					SizeBytes = (uint)type.SizeOf,
					AlignBytes = (uint)type.AlignOf
				};

				_builtinTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// レコード型を処理
			/// </summary>
			private void ProcessRecordType(CXCursor cursor)
			{
				if (!cursor.IsDefinition)
					return;

				var fields = new List<FieldInfo>();
				var methods = new List<MethodInfo>();
				var baseClasses = new List<string>();
				var nestedTypes = new List<RecordTypeInfo>();

				// 基底クラスを収集
				for (uint i = 0; i < cursor.NumBases; i++)
				{
					var baseCursor = cursor.GetBase(i);
					baseClasses.Add(baseCursor.Type.Spelling.CString);
				}

				// メンバーを収集
				cursor.VisitChildren((child, parent, data) =>
				{
					switch (child.Kind)
					{
						case CXCursorKind.CXCursor_FieldDecl:
							fields.Add(new FieldInfo
							{
								Name = child.Spelling.CString,
								Type = child.Type.Spelling.CString,
								Access = ConvertAccessLevel(child.CXXAccessSpecifier),
								IsStatic = child.IsStatic,
								IsConst = child.Type.IsConstQualified,
								IsMutable = child.CXXField_IsMutable,
								OffsetBits = child.OffsetOfField,
								BitFieldWidth = (uint)child.FieldDeclBitWidth
							});
							break;

						case CXCursorKind.CXCursor_CXXMethod:
						case CXCursorKind.CXCursor_Constructor:
						case CXCursorKind.CXCursor_Destructor:
							methods.Add(new MethodInfo
							{
								Name = child.Spelling.CString,
								Signature = child.Type.Spelling.CString,
								Access = ConvertAccessLevel(child.CXXAccessSpecifier),
								IsStatic = child.CXXMethod_IsStatic,
								IsVirtual = child.CXXMethod_IsVirtual,
								IsPure = child.CXXMethod_IsPureVirtual,
								IsConst = child.Type.IsConstQualified,
								IsOverride = false, // ClangSharpでは直接取得不可
								IsNoExcept = child.Type.ExceptionSpecificationType == CXCursor_ExceptionSpecificationKind.CXCursor_ExceptionSpecificationKind_BasicNoexcept
							});
							break;

						case CXCursorKind.CXCursor_ClassDecl:
						case CXCursorKind.CXCursor_StructDecl:
						case CXCursorKind.CXCursor_UnionDecl:
							ProcessRecordType(child);
							break;
					}

					return CXChildVisitResult.CXChildVisit_Continue;
				}, default);

				var location = cursor.Location;
				location.GetFileLocation(out var file, out var line, out var column, out _);

				var info = new RecordTypeInfo
				{
					Name = cursor.Spelling.CString,
					FullName = GetFullyQualifiedName(cursor),
					Namespace = GetNamespace(cursor),
					SourceFile = file.Name.CString,
					Line = line,
					Column = column,
					OriginalCursor = cursor,
					OriginalType = cursor.Type,
					Kind = cursor.Kind switch
					{
						CXCursorKind.CXCursor_ClassDecl => RecordTypeInfo.RecordKind.Class,
						CXCursorKind.CXCursor_StructDecl => RecordTypeInfo.RecordKind.Struct,
						CXCursorKind.CXCursor_UnionDecl => RecordTypeInfo.RecordKind.Union,
						_ => RecordTypeInfo.RecordKind.Class
					},
					AccessLevel = ConvertAccessLevel(cursor.CXXAccessSpecifier),
					Fields = fields.AsReadOnly(),
					Methods = methods.AsReadOnly(),
					BaseClasses = baseClasses.AsReadOnly(),
					NestedTypes = nestedTypes.AsReadOnly(),
					IsTemplated = cursor.IsTemplated,
					IsComplete = cursor.Type.SizeOf >= 0,
					SizeBytes = cursor.Type.SizeOf,
					AlignBytes = (uint)cursor.Type.AlignOf
				};

				_recordTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// 列挙型を処理
			/// </summary>
			private void ProcessEnumType(CXCursor cursor)
			{
				if (!cursor.IsDefinition)
					return;

				var values = new List<EnumTypeInfo.EnumValue>();

				// 列挙値を収集
				for (uint i = 0; i < cursor.NumEnumerators; i++)
				{
					var enumCursor = cursor.GetEnumerator(i);
					values.Add(new EnumTypeInfo.EnumValue
					{
						Name = enumCursor.Spelling.CString,
						Value = enumCursor.EnumConstantDeclValue,
						IsUnsigned = enumCursor.IsUnsigned
					});
				}

				var location = cursor.Location;
				location.GetFileLocation(out var file, out var line, out var column, out _);

				var info = new EnumTypeInfo
				{
					Name = cursor.Spelling.CString,
					FullName = GetFullyQualifiedName(cursor),
					Namespace = GetNamespace(cursor),
					SourceFile = file.Name.CString,
					Line = line,
					Column = column,
					OriginalCursor = cursor,
					OriginalType = cursor.Type,
					IsScoped = cursor.EnumDecl_IsScoped,
					UnderlyingType = cursor.EnumDecl_IntegerType.Spelling.CString,
					Values = values.AsReadOnly(),
					AccessLevel = ConvertAccessLevel(cursor.CXXAccessSpecifier)
				};

				_enumTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// テンプレート型を処理
			/// </summary>
			private void ProcessTemplateType(CXCursor cursor)
			{
				var parameters = new List<TemplateTypeInfo.TemplateParameter>();
				var specializations = new List<string>();

				// テンプレートパラメータを収集
				for (uint listIndex = 0; listIndex < cursor.NumTemplateParameterLists; listIndex++)
				{
					var numParams = cursor.GetNumTemplateParameters(listIndex);
					for (uint paramIndex = 0; paramIndex < numParams; paramIndex++)
					{
						var paramCursor = cursor.GetTemplateParameter(listIndex, paramIndex);
						parameters.Add(new TemplateTypeInfo.TemplateParameter
						{
							Name = paramCursor.Spelling.CString,
							Kind = paramCursor.Kind switch
							{
								CXCursorKind.CXCursor_TemplateTypeParameter => TemplateTypeInfo.TemplateParameter.ParameterKind.Type,
								CXCursorKind.CXCursor_NonTypeTemplateParameter => TemplateTypeInfo.TemplateParameter.ParameterKind.NonType,
								CXCursorKind.CXCursor_TemplateTemplateParameter => TemplateTypeInfo.TemplateParameter.ParameterKind.Template,
								_ => TemplateTypeInfo.TemplateParameter.ParameterKind.Type
							},
							DefaultValue = "" // ClangSharpでは直接取得が困難
						});
					}
				}

				// 特殊化を収集
				for (uint i = 0; i < cursor.NumSpecializations; i++)
				{
					var specCursor = cursor.GetSpecialization(i);
					specializations.Add(specCursor.Type.Spelling.CString);
				}

				var location = cursor.Location;
				location.GetFileLocation(out var file, out var line, out var column, out _);

				var info = new TemplateTypeInfo
				{
					Name = cursor.Spelling.CString,
					FullName = GetFullyQualifiedName(cursor),
					Namespace = GetNamespace(cursor),
					SourceFile = file.Name.CString,
					Line = line,
					Column = column,
					OriginalCursor = cursor,
					OriginalType = cursor.Type,
					Parameters = parameters.AsReadOnly(),
					Specializations = specializations.AsReadOnly(),
					Kind = cursor.Kind switch
					{
						CXCursorKind.CXCursor_ClassTemplate => TemplateTypeInfo.TemplateKind.Class,
						CXCursorKind.CXCursor_FunctionTemplate => TemplateTypeInfo.TemplateKind.Function,
						CXCursorKind.CXCursor_TypeAliasTemplateDecl => TemplateTypeInfo.TemplateKind.TypeAlias,
						_ => TemplateTypeInfo.TemplateKind.Class
					}
				};

				_templateTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// Typedef/型エイリアスを処理
			/// </summary>
			private void ProcessTypedefType(CXCursor cursor)
			{
				var location = cursor.Location;
				location.GetFileLocation(out var file, out var line, out var column, out _);

				var info = new TypedefInfo
				{
					Name = cursor.Spelling.CString,
					FullName = GetFullyQualifiedName(cursor),
					Namespace = GetNamespace(cursor),
					SourceFile = file.Name.CString,
					Line = line,
					Column = column,
					OriginalCursor = cursor,
					OriginalType = cursor.Type,
					UnderlyingType = cursor.TypedefDeclUnderlyingType.Spelling.CString,
					IsTemplateAlias = cursor.Kind == CXCursorKind.CXCursor_TypeAliasDecl
				};

				_typedefTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// 関数型を処理
			/// </summary>
			private void ProcessFunctionType(CXCursor cursor)
			{
				var parameters = new List<FunctionTypeInfo.ParameterInfo>();

				// パラメータを収集
				for (uint i = 0; i < cursor.NumArguments; i++)
				{
					var argCursor = cursor.GetArgument(i);
					parameters.Add(new FunctionTypeInfo.ParameterInfo
					{
						Name = argCursor.Spelling.CString,
						Type = argCursor.Type.Spelling.CString,
						HasDefault = argCursor.HasDefaultArg
					});
				}

				var location = cursor.Location;
				location.GetFileLocation(out var file, out var line, out var column, out _);

				var info = new FunctionTypeInfo
				{
					Name = cursor.Spelling.CString,
					FullName = GetFullyQualifiedName(cursor),
					Namespace = GetNamespace(cursor),
					SourceFile = file.Name.CString,
					Line = line,
					Column = column,
					OriginalCursor = cursor,
					OriginalType = cursor.Type,
					ReturnType = cursor.ResultType.Spelling.CString,
					Parameters = parameters.AsReadOnly(),
					IsVariadic = cursor.Type.IsFunctionTypeVariadic,
					CallingConv = FunctionTypeInfo.CallingConvention.C, // 簡略化
					IsNoExcept = cursor.Type.ExceptionSpecificationType == CXCursor_ExceptionSpecificationKind.CXCursor_ExceptionSpecificationKind_BasicNoexcept
				};

				_functionTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// 関数型（型から）を処理
			/// </summary>
			private void ProcessFunctionTypeFromType(CXType type)
			{
				var parameters = new List<FunctionTypeInfo.ParameterInfo>();

				// パラメータ型を収集
				for (uint i = 0; i < type.NumArgTypes; i++)
				{
					var argType = type.GetArgType(i);
					parameters.Add(new FunctionTypeInfo.ParameterInfo
					{
						Name = $"param{i}",
						Type = argType.Spelling.CString,
						HasDefault = false
					});
				}

				var info = new FunctionTypeInfo
				{
					Name = type.Spelling.CString,
					FullName = type.Spelling.CString,
					Namespace = "",
					SourceFile = "<built-in>",
					Line = 0,
					Column = 0,
					OriginalCursor = default,
					OriginalType = type,
					ReturnType = type.ResultType.Spelling.CString,
					Parameters = parameters.AsReadOnly(),
					IsVariadic = type.IsFunctionTypeVariadic,
					CallingConv = FunctionTypeInfo.CallingConvention.C,
					IsNoExcept = false
				};

				_functionTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// ポインタ/参照型を処理
			/// </summary>
			private void ProcessPointerType(CXType type)
			{
				var qualifiers = new List<string>();

				if (type.IsConstQualified) qualifiers.Add("const");
				if (type.IsVolatileQualified) qualifiers.Add("volatile");
				if (type.IsRestrictQualified) qualifiers.Add("restrict");

				var info = new PointerTypeInfo
				{
					Name = type.Spelling.CString,
					FullName = type.Spelling.CString,
					Namespace = "",
					SourceFile = "<built-in>",
					Line = 0,
					Column = 0,
					OriginalCursor = default,
					OriginalType = type,
					PointeeType = type.PointeeType.Spelling.CString,
					Kind = type.kind switch
					{
						CXTypeKind.CXType_Pointer => PointerTypeInfo.PointerKind.Pointer,
						CXTypeKind.CXType_LValueReference => PointerTypeInfo.PointerKind.LValueReference,
						CXTypeKind.CXType_RValueReference => PointerTypeInfo.PointerKind.RValueReference,
						_ => PointerTypeInfo.PointerKind.Pointer
					},
					Qualifiers = qualifiers.AsReadOnly()
				};

				_pointerTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// 配列型を処理
			/// </summary>
			private void ProcessArrayType(CXType type)
			{
				var info = new ArrayTypeInfo
				{
					Name = type.Spelling.CString,
					FullName = type.Spelling.CString,
					Namespace = "",
					SourceFile = "<built-in>",
					Line = 0,
					Column = 0,
					OriginalCursor = default,
					OriginalType = type,
					ElementType = type.ElementType.Spelling.CString,
					Kind = type.kind switch
					{
						CXTypeKind.CXType_ConstantArray => ArrayTypeInfo.ArrayKind.ConstantSize,
						CXTypeKind.CXType_IncompleteArray => ArrayTypeInfo.ArrayKind.IncompleteSize,
						CXTypeKind.CXType_DependentSizedArray => ArrayTypeInfo.ArrayKind.VariableSize,
						_ => ArrayTypeInfo.ArrayKind.ConstantSize
					},
					Size = type.ArraySize
				};

				_arrayTypes.Add(info);
				_typeCache[info.FullName] = info;
			}

			/// <summary>
			/// 変数型を処理（型情報収集のため）
			/// </summary>
			private void ProcessVariableType(CXCursor cursor)
			{
				// 変数の型を解析
				AnalyzeType(cursor.Type);
			}

			/// <summary>
			/// アクセスレベルを変換
			/// </summary>
			private static AccessLevel ConvertAccessLevel(CX_CXXAccessSpecifier access)
			{
				return access switch
				{
					CX_CXXAccessSpecifier.CX_CXXPublic => AccessLevel.Public,
					CX_CXXAccessSpecifier.CX_CXXProtected => AccessLevel.Protected,
					CX_CXXAccessSpecifier.CX_CXXPrivate => AccessLevel.Private,
					_ => AccessLevel.Public
				};
			}

			/// <summary>
			/// 完全修飾名を取得
			/// </summary>
			private static string GetFullyQualifiedName(CXCursor cursor)
			{
				var names = new List<string>();
				var current = cursor;

				while (!current.IsNull && current.Kind != CXCursorKind.CXCursor_TranslationUnit)
				{
					var name = current.Spelling.CString;
					if (!string.IsNullOrEmpty(name))
					{
						names.Add(name);
					}
					current = current.SemanticParent;
				}

				names.Reverse();
				return string.Join("::", names);
			}

			/// <summary>
			/// 名前空間を取得
			/// </summary>
			private static string GetNamespace(CXCursor cursor)
			{
				var namespaces = new List<string>();
				var current = cursor.SemanticParent;

				while (!current.IsNull && current.Kind != CXCursorKind.CXCursor_TranslationUnit)
				{
					if (current.Kind == CXCursorKind.CXCursor_Namespace)
					{
						var name = current.Spelling.CString;
						if (!string.IsNullOrEmpty(name))
						{
							namespaces.Add(name);
						}
					}
					current = current.SemanticParent;
				}

				namespaces.Reverse();
				return string.Join("::", namespaces);
			}

            #endregion
		}

		// 使用例
		public static class Example
		{
			public static void Exe(string sourceFile, ReadOnlySpan<string> compilerArgs)
			{
				using var collector = new ComprehensiveTypeCollector();

                try
                {
                    // C++ファイルを解析
                    var result = collector.CollectAllTypes(sourceFile, compilerArgs);

                    // 統計を表示
                    collector.PrintStatistics();

                    StreamWriter writer = new StreamWriter("""C:\Users\<user>\Downloads\log.txt""", false, System.Text.Encoding.UTF8);
					// 収集された型情報を詳細表示
					writer.WriteLine("\n=== クラス型 ===");
                    foreach (var record in result.RecordTypes)
                    {
						writer.WriteLine($"{record.Kind}: {record.FullName}");
                        //	Console.WriteLine($"  フィールド数: {record.Fields.Count}");
                        //	Console.WriteLine($"  メソッド数: {record.Methods.Count}");
                        //	Console.WriteLine($"  サイズ: {record.SizeBytes} bytes");
                    }

					writer.WriteLine("\n=== 列挙型 ===");
                    foreach (var enumType in result.EnumTypes)
                    {
						writer.WriteLine($"Enum: {enumType.FullName}");
                        //	Console.WriteLine($"  基底型: {enumType.UnderlyingType}");
                        //	Console.WriteLine($"  値数: {enumType.Values.Count}");
                    }

					writer.WriteLine("\n=== TypeDef ===");
                    foreach (var template in result.TypedefTypes)
                    {
						writer.WriteLine($"TypeDef: {template.FullName}");
                        //	Console.WriteLine($"  パラメータ数: {template.Parameters.Count}");
                        //	Console.WriteLine($"  特殊化数: {template.Specializations.Count}");
                    }

                    writer.Dispose();

				}
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"Error: {ex.Message}");
                }
			}
		}
        #endregion

        #endregion
	}



}
#endif