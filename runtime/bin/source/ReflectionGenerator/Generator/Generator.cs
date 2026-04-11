using ClangSharp;
using static ReflectionGenerator.Parser2.ParseExtensions;
using static ReflectionGenerator.Parser.ClangSharpExtension;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;

namespace ReflectionGenerator.Generator
{
    /// <summary>
    /// コード出力
    /// </summary>
    public class Generator
    {
        #region 定義
        public readonly struct ARTIFACT_INFO : IEquatable<ARTIFACT_INFO>
        {
            /// <summary>
            /// ビルド
            /// </summary>
            public required bool Build { get; init; }

            /// <summary>
            /// リビルドフラグ
            /// </summary>
            public required bool ReBuild { get; init; } 

            /// <summary>
            /// モジュール名
            /// </summary>
            public required string ArtifactName { get; init; }

            //  ソリューションディレクトリ以下のパス
            public required string RelativePath { get; init; }

			public required bool IsModule { get; init; }

            public override int GetHashCode()
            {
                return ArtifactName.GetHashCode();
            }

            bool IEquatable<ARTIFACT_INFO>.Equals(ARTIFACT_INFO other)
            {
                return GetHashCode() == other.GetHashCode();
            }
        }

		private static class CPP_DEFINE
		{
			public const string INTELLISENSE = "__INTELLISENSE__";
		}
		#endregion

		#region 定数
		const string ClassInfoStr = "ClassDecl";
        const string GlobalDeclStr = "GlobalDecl";
        const int NUM_DIGIT_INDEX = 2;
        private const string NOX_REFLECTION_GEN_NAMESPACE_STR = "nox::reflection::gen";
		#endregion

		#region 非公開フィールド
		private string _BaseDirectory = string.Empty;

        private string _AdditionalModuleIncludeStr = string.Empty;
        #endregion

        #region 公開プロパティ
        /// <summary>
        /// Key: モジュール名 Value: モジュールごとの型情報リスト
        /// </summary>
        public required IReadOnlyDictionary<string, List<Parser2.NamespaceDecl>> TypeInfoListWithArtifactNameDict { get; init; }

        private IReadOnlyList<Parser2.NamespaceDecl> NamespaceDeclList
        {
            get
            {
                List<Parser2.NamespaceDecl> namespaceDeclList = new List<Parser2.NamespaceDecl>();
                foreach (var pair in TypeInfoListWithArtifactNameDict)
                {
                    namespaceDeclList.AddRange(pair.Value);
                }
                return namespaceDeclList;
			}
        }

		/// <summary>
		/// 1ファイル内の定義数
		/// </summary>
		public uint NumOfDefinitionInFile { private get; init; } = 30;

        /// <summary>
        /// 出力先ディレクトリ
        /// </summary>
        public required string OutputProjectDirectory { private get; init; }

        /// <summary>
        /// 出力先ディレクトリ
        /// </summary>
        public required string OutputDirectory { private get; init; }

        /// <summary>
        /// 構成名
        /// </summary>
        public required string Configuration { private get; init; }

        /// <summary>
        /// プラットフォーム名
        /// </summary>
        public required string Platform { private get; init; }

        /// <summary>
        /// 構成名のプリプロセッサ定義
        /// </summary>
        public required string ConfigurationDefine { private get; init; }

        /// <summary>
        /// プラットフォーム名のプリプロセッサ定義
        /// </summary>
        public required string PlatformDefine { private get; init; }

        public required ARTIFACT_INFO[] ModuleInfoList { get; init; }
        #endregion

        #region 非公開プロパティ
        #endregion

        #region 公開メソッド
		public unsafe bool Generate()
        {
			string baseGenHeaderFilePath = $"{OutputDirectory}/gen.h";

            //  出力先ディレクトリを決定
            _BaseDirectory = Path.GetFullPath($"{OutputDirectory}/{Platform}/{Configuration}/");

            if (Directory.Exists(_BaseDirectory) == false)
            {
                //  ディレクトリが存在しない場合作成
                Directory.CreateDirectory(_BaseDirectory);
            }


            for (int i = 0; i < ModuleInfoList.Length; ++i)
            {
                ref readonly ARTIFACT_INFO moduleInfo = ref ModuleInfoList[i];
                if (moduleInfo.IsModule == false)
                {
                    continue;
                }

                if (_AdditionalModuleIncludeStr == string.Empty)
                {
					_AdditionalModuleIncludeStr = $"#include\t\"../../../../{moduleInfo.RelativePath}\"";
				}
				else
                {
					_AdditionalModuleIncludeStr += $"\r\n#include\t\"../../../../{moduleInfo.RelativePath}\"";
				}
			}

			//  モジュールごとのヘッダファイルを生成
			//  ~/gen/BuildSpec/Platform/gen_BuildSpec_Platform_ModuleName.h
			{

                string genHeaderFilePath = System.IO.Path.GetFullPath($"{_BaseDirectory}/gen_{Platform}_{Configuration}.g.h");

                using (CodeWriter codeWriter = new CodeWriter(genHeaderFilePath))
                {
                    codeWriter.WriteLineCopyRight();
                    codeWriter.WriteLineHeader();

                    codeWriter.WriteLinePPIf(ConfigurationDefine);
                    codeWriter.WriteLinePPIf(PlatformDefine);

                    codeWriter.WriteNamespace(NOX_REFLECTION_GEN_NAMESPACE_STR);
                    codeWriter.PushScope(CodeWriter.ScopeType.Define);

                    for (int i = 0; i < NumOfDefinitionInFile; ++i)
                    {
                        string indexStr = i.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                        codeWriter.WriteLine($"void\tRegister_{Platform}_{Configuration}_{ClassInfoStr}_{indexStr}();");
                        codeWriter.WriteLine($"void\tUnregister_{Platform}_{Configuration}_{ClassInfoStr}_{indexStr}();");

                        codeWriter.WriteLine($"void\tRegister_{Platform}_{Configuration}_GlobalDecl_{indexStr}();");
                        codeWriter.WriteLine($"void\tUnregister_{Platform}_{Configuration}_GlobalDecl_{indexStr}();");
                    }

                    codeWriter.PopScope();

                    codeWriter.WriteLinePPEndIf(PlatformDefine);
                    codeWriter.WriteLinePPEndIf(ConfigurationDefine);
                }

				GenerateDeclaration();
			}

            //  モジュールごとのソースファイルを生成
            //GenerateDeclaration();

			//  ~/gen/BuildSpec/Platform/gen_BuildSpec_Platform_ModuleName_{index}.cpp

            //  統括ソースファイルを生成
            {
                string unitySourceFilePath = System.IO.Path.GetFullPath($"{_BaseDirectory}/gen_{Platform}_{Configuration}.g.cpp");
                using (CodeWriter codeWriter = new CodeWriter(unitySourceFilePath))
                {
                    codeWriter.WriteLineSource();
                    codeWriter.WriteNewLine();

					codeWriter.WriteIncludePch();
                    codeWriter.WriteLineInclude($"gen.h");

                    codeWriter.WriteNewLine();

                    codeWriter.WriteLinePPIf(ConfigurationDefine);
                    codeWriter.WriteLinePPIf(PlatformDefine);

                    //  インテリジェンス環境では無効にする
                    codeWriter.WriteLinePPIfNotDefine(CPP_DEFINE.INTELLISENSE);

                    //  最適化をOFF
                    codeWriter.WriteLine("#pragma optimize(\"\", off)");

					codeWriter.WriteLineInclude($"gen_{Platform}_{Configuration}.g.h");

					{
						// 登録
						codeWriter.WriteLine($"void ::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}()");
                        {
                            codeWriter.PushScope(CodeWriter.ScopeType.Define);
							//for (int i = 0; i < ModuleInfoList.Length; ++i)
                            {
                                //ref readonly ARTIFACT_INFO moduleInfo = ref ModuleInfoList[i];
                                //codeWriter.WriteLineRegion(moduleInfo.ArtifactName);
                                for (int divideIndex = 0; divideIndex < NumOfDefinitionInFile; ++divideIndex)
                                {
                                    string indexStr = divideIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_{ClassInfoStr}_{indexStr}();");
                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_GlobalDecl_{indexStr}();");
                                }
                                //codeWriter.WriteLineEndRegion(moduleInfo.ArtifactName);
                            }
							codeWriter.PopScope();
                        }

                        // 登録解除
                        codeWriter.WriteLine($"void ::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}()");
                        {
                            codeWriter.PushScope(CodeWriter.ScopeType.Define);
						//	for (int i = 0; i < ModuleInfoList.Length; ++i)
                            {
                            //    ref readonly ARTIFACT_INFO moduleInfo = ref ModuleInfoList[i];
                            //    codeWriter.WriteLineRegion(moduleInfo.ArtifactName);
                                for (int divideIndex = 0; divideIndex < NumOfDefinitionInFile; ++divideIndex)
                                {
                                    string indexStr = divideIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_{ClassInfoStr}_{indexStr}();");
                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_GlobalDecl_{indexStr}();");
                                }
                            //    codeWriter.WriteLineEndRegion(moduleInfo.ArtifactName);
                            }
							codeWriter.PopScope();
                        }
                    }

					codeWriter.WriteLine("#pragma optimize(\"\", on)");
					codeWriter.WriteLinePPEndIf(CPP_DEFINE.INTELLISENSE);
					codeWriter.WriteLinePPEndIf(PlatformDefine);
                    codeWriter.WriteLinePPEndIf(ConfigurationDefine);
                }
            }


            //  直列
            //#if true

            //            foreach (var pair in TypeInfoListWithModuleNameDict)
            //            {
            //                if (TargetModuleNameList.Contains(pair.Key) == false)
            //                {
            //                    continue;
            //                }

            //                Generate(pair.Key, pair.Value);
            //            }
            //#else
            //            System.Threading.Tasks.Parallel.ForEach(TypeInfoListWithModuleNameDict.Keys, key =>
            //            {
            //                if (TargetModuleNameList.Contains(key) == false)
            //                {
            //                    return;
            //                }

            //                Generate(key, TypeInfoListWithModuleNameDict[key]);
            //            }
            //            );
            //#endif

            return true;
        }
        #endregion

        #region 非公開メソッド
        private static int calcIndex(int index, int length, int numOfDivision)
        {
            if (numOfDivision <= 0)
            {
                return 0;
            }
            return Math.Min(index * numOfDivision / length, numOfDivision - 1);
		}

  //      private void GenerateDeclaration()
  //      {
  //          //  ソースファイルの作成
  //          for(int moduleIndex = 0, moduleLength = ModuleInfoList.Length; moduleIndex < moduleLength; ++moduleIndex)
  //          {
  //              ref readonly ARTIFACT_INFO moduleInfo = ref ModuleInfoList[moduleIndex];
  //              if (moduleInfo.Build == false)
  //              {
  //                  continue;
  //              }

		//		string artifactName = moduleInfo.ArtifactName;

		//		//  書き込み対象のリスト
		//		CodeWriter[] codeWriterWithClassList = new CodeWriter[NumOfDefinitionInFile];
		//		CodeWriter[] codeWriterWithGlobalList = new CodeWriter[NumOfDefinitionInFile];

		//		//  ファイル数で分割する
		//		for (int i = 0; i < NumOfDefinitionInFile; ++i)
		//		{

		//			{
		//				string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/{artifactName}/{artifactName}_{ClassInfoStr}_{i}.cpp");
		//				codeWriterWithClassList[i] = new CodeWriter(path);
		//			}

		//			{
		//				string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/{artifactName}/{artifactName}_GlobalDecl_{i}.cpp");
		//				codeWriterWithGlobalList[i] = new CodeWriter(path);
		//			}

		//		}

		//		int useThreadCount = Util.MAX_THREAD_ID;
		//		useThreadCount = 1;    //  デバッグのため、スレッド数を1に固定
		//		{
		//			//MEMO 追加のインクルードディレクトリにプロジェクトディレクトリを指定している必要がある
		//			string baseHeaderFilePath = $"gen_{Platform}_{Configuration}_{artifactName}.h";

  //                  string additionalIncludeDir;
  //                  if (moduleInfo.IsModule == true)
  //                  {
  //                     additionalIncludeDir = $"../../../../../{moduleInfo.ArtifactName}/{moduleInfo.ArtifactName}.h";
  //                  }
  //                  else
  //                  {
  //                      additionalIncludeDir = string.Empty;
  //                  }

  //                      Util.ParallelFor(0, codeWriterWithGlobalList.Length + codeWriterWithClassList.Length, (int index) =>
  //                      {
  //                          CodeWriter codeWriter;
  //                          if (index < codeWriterWithGlobalList.Length)
  //                          {
  //                              //  グローバル宣言
  //                              codeWriter = codeWriterWithGlobalList[index];
  //                          }
  //                          else
  //                          {
  //                              //  クラス宣言
  //                              codeWriter = codeWriterWithClassList[index - codeWriterWithGlobalList.Length];
  //                          }

  //                          codeWriter.WriteLineSource();
  //                          codeWriter.WriteNewLine();
  //                          codeWriter.WriteIncludeStdafx();
  //                          codeWriter.WriteLineInclude(baseHeaderFilePath);

  //                          if (additionalIncludeDir != string.Empty)
  //                          {
  //                              codeWriter.WriteLineInclude(additionalIncludeDir);
  //                          }

  //                          codeWriter.WriteNewLine();

  //                          //  プリプロセッサ
  //                          codeWriter.WriteLinePPIf(ConfigurationDefine);
  //                          codeWriter.WriteLinePPIf(PlatformDefine);
  //                          codeWriter.WriteLinePPIfNot(CPP_DEFINE.INTELLISENSE);

  //                          //codeWriter.WriteLine($"#include\t\"../../../../support_functions.h\"");
  //                          //foreach (ReadOnlySpan<char> headerFileName in moduleInfo.IncludeHeaderList)
  //                          {
  //                              //	codeWriter.WriteLineInclude(headerFileName);
  //                          }

  //                          codeWriter.WriteNewLine();

  //                          //  定義
  //                          codeWriter.WriteLine($"namespace {NOX_REFLECTION_GEN_NAMESPACE_STR}");
  //                          codeWriter.WriteLine("{");
  //                          codeWriter.Push();
  //                      },
  //                      useThreadCount
  //                      );
		//		}

  //              {
		//			TypeInfoListWithArtifactNameDict.TryGetValue(artifactName, out List<Parser2.NamespaceDecl>? namespaceDeclList);
		//			GenerateWithArtifact(artifactName, namespaceDeclList, codeWriterWithGlobalList, codeWriterWithClassList);
		//		}

		//		Util.ParallelFor(0, codeWriterWithGlobalList.Length + codeWriterWithClassList.Length, (int index) =>
		//		{
		//			CodeWriter codeWriter;
		//			if (index < codeWriterWithGlobalList.Length)
		//			{
		//				//  グローバル宣言
		//				codeWriter = codeWriterWithGlobalList[index];
		//			}
		//			else
		//			{
		//				//  クラス宣言
		//				codeWriter = codeWriterWithClassList[index - codeWriterWithGlobalList.Length];
		//			}

		//			codeWriter.Pop();
		//			codeWriter.WriteLine("}");
		//			codeWriter.WriteLinePPEndIf(CPP_DEFINE.INTELLISENSE);
		//			codeWriter.WriteLinePPEndIf(PlatformDefine);
		//			codeWriter.WriteLinePPEndIf(ConfigurationDefine);
		//		},
		//		useThreadCount);


		//		foreach (CodeWriter codeWriter in codeWriterWithGlobalList)
		//		{
		//			codeWriter.Dispose();
		//		}
		//		foreach (CodeWriter codeWriter in codeWriterWithClassList)
		//		{
		//			codeWriter.Dispose();
		//		}
		//	}
		//}

        private void GenerateDeclaration()
        {
			//  ソースファイルの作成
			{
				//  書き込み対象のリスト
				CodeWriter[] codeWriterWithClassList = new CodeWriter[NumOfDefinitionInFile];
				CodeWriter[] codeWriterWithGlobalList = new CodeWriter[NumOfDefinitionInFile];

				//  ファイル数で分割する
				for (int i = 0; i < NumOfDefinitionInFile; ++i)
				{

					{
						string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/class_decl_{i}.g.cpp");
						codeWriterWithClassList[i] = new CodeWriter(path);
					}

					{
						string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/global_decl_{i}.g.cpp");
						codeWriterWithGlobalList[i] = new CodeWriter(path);
					}

				}

				int useThreadCount = -1;
				//useThreadCount = 1;    //  デバッグのため、スレッド数を1に固定
				{
					string baseHeaderFilePath = $"gen_{Platform}_{Configuration}.g.h";

					Util.ParallelFor(0, codeWriterWithGlobalList.Length + codeWriterWithClassList.Length, (int index) =>
					{
						CodeWriter codeWriter;
						if (index < codeWriterWithGlobalList.Length)
						{
							//  グローバル宣言
							codeWriter = codeWriterWithGlobalList[index];
						}
						else
						{
							//  クラス宣言
							codeWriter = codeWriterWithClassList[index - codeWriterWithGlobalList.Length];
						}

						codeWriter.WriteLineSource();
						codeWriter.WriteNewLine();
						codeWriter.WriteIncludePch();
						codeWriter.WriteLineInclude(baseHeaderFilePath);
                        codeWriter.WriteLine(_AdditionalModuleIncludeStr);

						codeWriter.WriteNewLine();

						//  プリプロセッサ
						codeWriter.WriteLinePPIf(ConfigurationDefine);
						codeWriter.WriteLinePPIf(PlatformDefine);
						codeWriter.WriteLinePPIfNot(CPP_DEFINE.INTELLISENSE);

						codeWriter.WriteLine("#pragma optimize(\"\", off)");

						//codeWriter.WriteLine($"#include\t\"../../../../support_functions.h\"");
						//foreach (ReadOnlySpan<char> headerFileName in moduleInfo.IncludeHeaderList)
						{
							//	codeWriter.WriteLineInclude(headerFileName);
						}

						codeWriter.WriteNewLine();

						//  定義
						codeWriter.WriteLine($"namespace {NOX_REFLECTION_GEN_NAMESPACE_STR}");
						codeWriter.WriteLine("{");
						codeWriter.Push();
					},
					useThreadCount
					);
				}

				{
					GenerateWithArtifact(NamespaceDeclList, codeWriterWithGlobalList, codeWriterWithClassList);
				}

				Util.ParallelFor(0, codeWriterWithGlobalList.Length + codeWriterWithClassList.Length, (int index) =>
				{
					CodeWriter codeWriter;
					if (index < codeWriterWithGlobalList.Length)
					{
						//  グローバル宣言
						codeWriter = codeWriterWithGlobalList[index];
					}
					else
					{
						//  クラス宣言
						codeWriter = codeWriterWithClassList[index - codeWriterWithGlobalList.Length];
					}

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("#pragma optimize(\"\", on)");
					codeWriter.WriteLinePPEndIf(CPP_DEFINE.INTELLISENSE);
					codeWriter.WriteLinePPEndIf(PlatformDefine);
					codeWriter.WriteLinePPEndIf(ConfigurationDefine);
				},
				useThreadCount);


				foreach (CodeWriter codeWriter in codeWriterWithGlobalList)
				{
					codeWriter.Dispose();
				}
				foreach (CodeWriter codeWriter in codeWriterWithClassList)
				{
					codeWriter.Dispose();
				}
			}

		}

		private void GenerateWithArtifact(IReadOnlyList<Parser2.NamespaceDecl> namespaceDeclInfoList, CodeWriter[] codeWriterWithGlobalList, CodeWriter[] codeWriterWithClassList)
        {
            int useThreadCount = Util.MAX_THREAD_ID;
            useThreadCount = 1;    //  デバッグのため、スレッド数を1に固定

            List<Parser2.VariableDecl> globalVariableInfoList = new();
            List<Parser2.FunctionDecl> globalFunctionInfoList = new();
            List<Parser2.EnumDecl> globalEnumInfoList = new();
            List<Parser2.RecordDecl> classInfoList = new();

            //  定義を収集
            using(new ScopeProfiler() { Tag = "CollectDeclarations" })
			{
                void probe(Parser2.NamespaceDecl namespaceDeclInfo)
                {
                    if (
						namespaceDeclInfo.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.None ||
						namespaceDeclInfo.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection
                        )
                    {
                        return;
                    }

					namespaceDeclInfo.VariableList.ForEach(
                        x =>
                        {
                            if (x.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
                            {
                                return;
                            }

                            globalVariableInfoList.Add(x);
                        }
                        );

                    namespaceDeclInfo.FunctionList.ForEach(
                        x =>
                        {
                            if (x.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
                            {
                                return;
                            }
                            globalFunctionInfoList.Add(x);
                        }
                        );

                    namespaceDeclInfo.EnumList.ForEach(
                        x =>
                        {
							if (x.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
							{
								return;
							}
                            globalEnumInfoList.Add(x);
                        }
                        );

                    namespaceDeclInfo.RecordList.ForEach(
                        x =>
                        {
                            if (x.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
                            {
                                return;
                            }
                            classInfoList.Add(x);
                        }
                        );
                }

                namespaceDeclInfoList.ForEach(probe);
            }

            //  グローバル定義の総数
            int globalDeclarationLength = globalVariableInfoList.Count + globalFunctionInfoList.Count + globalEnumInfoList.Count;

            Stack<CodeStringBuilder> codeStringBuilderStack = new Stack<CodeStringBuilder>();
            for(int i = 0; i < 10; ++i)
            {
				codeStringBuilderStack.Push(new CodeStringBuilder());
			}

            object lockCodeStringBuildStack = new object();
			CodeStringBuilder issueCodeStringBuilder()
            {
                lock(lockCodeStringBuildStack)
                {
                    if(codeStringBuilderStack.Count > 0)
                    {
                        return codeStringBuilderStack.Pop();
					}
				}
				CodeStringBuilder codeStringBuilder = new CodeStringBuilder();

                return codeStringBuilder;
			}

            void returnCodeStringBuilder(CodeStringBuilder codeStringBuilder)
            {
				codeStringBuilder.Clear();
				lock (lockCodeStringBuildStack)
                {
                    codeStringBuilderStack.Push(codeStringBuilder);
                }
            }

            //  宣言の登録バッファリスト
            List<string>[] registerDeclListTableWithGlobal = new List<string>[codeWriterWithGlobalList.Length];
            for (int i = 0; i < registerDeclListTableWithGlobal.Length; ++i)
            {
                registerDeclListTableWithGlobal[i] = new List<string>();
            }

            List<string>[] registerDeclListTableWithClass = new List<string>[codeWriterWithClassList.Length];
            for (int i = 0; i < registerDeclListTableWithClass.Length; ++i)
            {
                registerDeclListTableWithClass[i] = new List<string>();
			}

			Util.ParallelFor(0, globalDeclarationLength + classInfoList.Count, (int index) => 
            {
				CodeStringBuilder codeStringBuilder = issueCodeStringBuilder();
                using (codeStringBuilder.Indent())
                {
                    if (index < globalDeclarationLength)
                    {
                        //  グローバル宣言
                        int globalDeclarationIndex = index;
                        string declName;
                        if (globalDeclarationIndex < globalVariableInfoList.Count)
                        {
                            //  変数宣言
                            Parser2.VariableDecl variableInfo = globalVariableInfoList[globalDeclarationIndex];
                            if (variableInfo.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
                            {
                                return;
                            }

                            GenerateVariableInfo(codeStringBuilder, variableInfo, null);
							declName = $"variable_info_{variableInfo.Hash.ToString()}";

                        }
                        else if (globalDeclarationIndex < globalVariableInfoList.Count + globalFunctionInfoList.Count)
                        {
                            //  関数宣言
                            Parser2.FunctionDecl functionInfo = globalFunctionInfoList[globalDeclarationIndex - globalVariableInfoList.Count];
                            if (functionInfo.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
                            {
                                return;
                            }

                            GenerateFunctionInfo(codeStringBuilder, functionInfo, null);
                            declName = $"function_info_{functionInfo.Hash.ToString()}";
                        }
                        else
                        {
                            //  Enum宣言
                            Parser2.EnumDecl enumInfo = globalEnumInfoList[globalDeclarationIndex - globalVariableInfoList.Count - globalFunctionInfoList.Count];
                            GenerateEnumInfo(codeStringBuilder, enumInfo, null);
                            declName = $"enum_info_{enumInfo.Hash.ToString()}";
                        }

                        //  書き込むコードのインデックスを取得
                        int codeWriterWithGlobalIndex = calcIndex(globalDeclarationIndex, globalDeclarationLength, codeWriterWithGlobalList.Length);
                        CodeWriter codeWriterWithGlobal = codeWriterWithGlobalList[codeWriterWithGlobalIndex];

                        //  CodeWriterへ登録
                        lock (codeWriterWithGlobal)
                        {
                            codeWriterWithGlobal.WriteLine(codeStringBuilder.ToString());
                            registerDeclListTableWithGlobal[codeWriterWithGlobalIndex].Add(declName);
                        }
                    }
                    else
                    {
                        //  クラス宣言
                        int classDeclarationIndex = index - globalDeclarationLength;
                        int codeWriterWithClassIndex = calcIndex(classDeclarationIndex, classInfoList.Count, codeWriterWithClassList.Length);
                        Parser2.RecordDecl classInfo = classInfoList[classDeclarationIndex];

                        if (classInfo.Namespace.StartsWith("nox") == false)
                        {
                            //  未対応
                            return;
                        }
						CodeWriter codeWriter = codeWriterWithClassList[codeWriterWithClassIndex];

						List<string> registerClassDeclNameList = new List<string>();
                        GenerateClassInfo(codeStringBuilder, classInfo, classInfo, registerClassDeclNameList, null);

                        lock (codeWriter)
                        {
                            string tmp = codeStringBuilder.ToString();
                            if (tmp != string.Empty)
                            {
                                codeWriter.WriteLine(tmp);
                            }
                            registerDeclListTableWithClass[codeWriterWithClassIndex].AddRange(registerClassDeclNameList);
                        }
                    }

                }
				returnCodeStringBuilder(codeStringBuilder);
			});

            //  
            Util.ParallelFor(0, codeWriterWithGlobalList.Length + codeWriterWithClassList.Length,
                (int index) => 
                {
                    CodeWriter codeWriter;
                    IReadOnlyList<string> declarationList;

                    ReadOnlySpan<char> declStrRegister;
                    ReadOnlySpan<char> declStrUnregister;
					if (index < codeWriterWithGlobalList.Length)
                    {
                        //int subIndex = index / globalDivRem.Quotient;
                        //  書き込むコードのインデックスを取得
                        int codeWriterWithGlobalIndex = index;

						codeWriter = codeWriterWithGlobalList[codeWriterWithGlobalIndex];
                        declarationList = registerDeclListTableWithGlobal[codeWriterWithGlobalIndex];

						ReadOnlySpan<char> indexStr = codeWriterWithGlobalIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

						declStrRegister = $"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_GlobalDecl_{indexStr}()";
						declStrUnregister =$"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_GlobalDecl_{indexStr}()";
					}
                    else
                    {
						int classDeclarationIndex = index - codeWriterWithGlobalList.Length;
                        int codeWriterWithClassIndex = classDeclarationIndex;
						codeWriter = codeWriterWithClassList[codeWriterWithClassIndex];

                        declarationList = registerDeclListTableWithClass[codeWriterWithClassIndex];

						ReadOnlySpan<char> indexStr = codeWriterWithClassIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

						declStrRegister = $"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_ClassDecl_{indexStr}()";
						declStrUnregister = $"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_ClassDecl_{indexStr}()";
					}

					//  登録処理
					codeWriter.WriteLine(declStrRegister);
                    codeWriter.WriteLine("{");
                    using (codeWriter.Indent())
                    {
                        foreach (ReadOnlySpan<char> s in declarationList)
                        {
                            codeWriter.WriteLine($"nox::reflection::Register({s});");
                        }
                    }
                    codeWriter.WriteLine("}");

                    codeWriter.WriteNewLine();

					//  登録解除処理
					codeWriter.WriteLine(declStrUnregister);
					codeWriter.WriteLine("{");
                    using (codeWriter.Indent())
                    {

                        foreach (ReadOnlySpan<char> s in declarationList)
                        {
                            codeWriter.WriteLine($"nox::reflection::Unregister({s});");
                        }
                    }
					codeWriter.WriteLine("}");
				},
                useThreadCount);
        }

        private static string GetVariableInfoDeclName(Parser2.VariableDecl variableInfo) => $"variable_info_{variableInfo.Hash}";

        #region 定義生成群
        private int GenerateAttributeInfoList(BaseCodeWriter codeWriter, ReadOnlySpan<Parser2.AttributeDecl> attributeInfoList, ReadOnlySpan<char> hash)
        {
            int attributeInfoListLength = attributeInfoList.Length;
            if (attributeInfoListLength == 0)
            {
                return 0;
			}

			List<string> tmpDeclNameList = new List<string>();
            for (int i = 0, length = attributeInfoList.Length; i < length; ++i)
            {
                Parser2.AttributeDecl attributeInfo = attributeInfoList[i];
                string attr_decl_name = $"attribute_info_{attributeInfo.Hash}_{i.ToString()}";

                switch(attributeInfo.AttrKind)
                {
                    case Parser2.AttrKind.EngineAnnotate:
						string fullDecl = attributeInfo.Value;
						codeWriter.WriteLine($"static constexpr decltype(auto) {attr_decl_name} = {fullDecl};");
						break;

                    case Parser2.AttrKind.ReflectionTarget:
                    case Parser2.AttrKind.IgnoreReflectionTarget:
                        continue;

                    default:
						ReadOnlySpan<char> standardAttrKindName = attributeInfo.AttrKind switch
						{
							Parser2.AttrKind.NoDiscard => "nox::reflection::StandardAttrKind::NoDiscard",
							_ => "nox::reflection::StandardAttrKind::Invalid"
						};

						codeWriter.WriteLine($"static constexpr nox::reflection::attr::StandardAttribute {attr_decl_name} = nox::reflection::attr::StandardAttribute({standardAttrKindName});\t//\t{attributeInfo.AttrName}({attributeInfo.Value})");
						break;
                }

				tmpDeclNameList.Add(attr_decl_name);
			}

            int tmpDeclNameListLength = tmpDeclNameList.Count;
            if (tmpDeclNameListLength <= 0)
            {
                return 0;
            }

			codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::ReflectionObject> attribute_info_table_{hash}[{tmpDeclNameListLength.ToString()}]={{");
            using (codeWriter.Indent())
            {
                for (int i = 0, length = tmpDeclNameList.Count; i < length; ++i)
                {
                    string declName = tmpDeclNameList[i];
                    if (i == length - 1)
                    {
                        codeWriter.WriteLine(declName);
                    }
                    else
                    {
                        codeWriter.WriteLine($"{declName},");
                    }
                }
            }
			codeWriter.WriteLine("};");
            codeWriter.WriteNewLine();

            return tmpDeclNameListLength;
		}

        private bool GenerateVariableInfo(BaseCodeWriter codeWriter, Parser2.VariableDecl variableInfo, Parser2.RecordDecl? declarationTypeInfo)
        {
			//  属性定義
			int enabledAttributeLength;
            {
                ReadOnlySpan<Parser2.AttributeDecl> attributeList = variableInfo.AttributeSpan;
				if (attributeList.Length > 0)
				{
					enabledAttributeLength = GenerateAttributeInfoList(codeWriter, attributeList, variableInfo.Hash);
				}
				else
				{
					enabledAttributeLength = 0;
				}
			}

			//  setter, getterの有無
			bool noSetter =
				variableInfo.Type.TypeKind == Parser2.TypeKind.BoundedArray || variableInfo.Type.TypeKind == Parser2.TypeKind.UnboundedArray ||
				variableInfo.Type.TypeAttributeFlags.IsOn(Parser2.TypeAttributeFlag.Const) == true
                ;

            bool noGetter =
				variableInfo.Type.TypeKind == Parser2.TypeKind.BoundedArray || variableInfo.Type.TypeKind == Parser2.TypeKind.UnboundedArray
				;

			//  アドレスを取得できない変数かどうか
			bool noAddressOf = 
                variableInfo.Type.TypeAttributeFlags.IsAnyOn(Parser2.TypeAttributeFlag.LValueReference | Parser2.TypeAttributeFlag.RValueReference) ||
				variableInfo.BitFieldWidth >= 0
				;

            //  メンバ変数
            bool isStatic = variableInfo.VariableAttributeFlags.IsOn(Parser2.VariableAttributeFlag.Static);
			if (isStatic == false)
			{
				if (noAddressOf)
				{
					codeWriter.WriteLine($"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoMemberRef<decltype({variableInfo.FullName})>");
				}
				else
				{
					codeWriter.WriteLine($"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoMember<decltype({variableInfo.FullName}), &{variableInfo.FullName}>");
				}
			}
			//  グローバル変数
			else
			{
				if (noAddressOf)
				{
					codeWriter.WriteLine($"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoGlobalRef<decltype({variableInfo.FullName})>");
				}
				else
				{
					codeWriter.WriteLine($"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoGlobal<decltype({variableInfo.FullName}), &{variableInfo.FullName}>");
				}
			}
            
            codeWriter.WriteLine("(");
            using (codeWriter.Indent())
            {
                if (!noAddressOf)
                {
                    codeWriter.WriteLine($"nox::GetObjectPointerId<&{variableInfo.FullName}>(),\t//\tobject_pointer_id");
                }
                else
                {
					codeWriter.WriteLine($"nox::reflection::Typeof<decltype({variableInfo.FullName})>(),\t//\ttype");
					if (!isStatic)
					{
						System.Diagnostics.Debug.Assert(declarationTypeInfo != null, "declarationTypeInfo must not be null for non-static variables.");
						codeWriter.WriteLine($"nox::reflection::Typeof<{declarationTypeInfo.FullName}>(),\t//\towner_type");
					}
				}

				codeWriter.WriteLine($"u8\"{variableInfo.Name}\",\t//\tname");
                codeWriter.WriteLine($"u8\"{variableInfo.FullName}\",\t//\tfullName");
                codeWriter.WriteLine($"u8\"{variableInfo.Namespace}\",\t//\tnamespace");

                codeWriter.WriteLine($"{variableInfo.AccessLevel.GetRuntimeFqn()},\t//\taccess level");
                codeWriter.WriteLine($"{variableInfo.BitFieldWidth.ToString()},\t//\tbit with");
                codeWriter.WriteLine($"{variableInfo.OffsetBits.ToString()},\t//\toffset");

                //  属性
                if (enabledAttributeLength > 0)
                {
                    codeWriter.WriteLine($"attribute_info_table_{variableInfo.Hash},\t//\tattribute");
                }
                else
                {
                    codeWriter.WriteLine("nullptr,\t//\tattribute");
                }

                codeWriter.WriteLine($"{enabledAttributeLength.ToString()},\t//\tattribute length");

                //  変数属性
                {
                    List<string> variableAttributeFlagsStrList = [];
                    if (variableInfo.VariableAttributeFlags.IsOn(Parser2.VariableAttributeFlag.Constexpr) == true)
                    {
                        variableAttributeFlagsStrList.Add("nox::reflection::VariableAttributeFlag::Constexpr");
                    }
                    if (variableInfo.VariableAttributeFlags.IsOn(Parser2.VariableAttributeFlag.Static) == true)
                    {
                        variableAttributeFlagsStrList.Add("nox::reflection::VariableAttributeFlag::Static");
                    }

                    if (variableAttributeFlagsStrList.Count > 0)
                    {
                        if (variableAttributeFlagsStrList.Count == 1)
                        {
                            codeWriter.WriteLine($"{variableAttributeFlagsStrList[0]},\t//\tvariable attribute flags");
                        }
                        else
                        {
                            codeWriter.WriteLine("nox::util::BitOr(");
                            for (int i = 0, length = variableAttributeFlagsStrList.Count; i < variableAttributeFlagsStrList.Count; ++i)
                            {
                                if (i != length - 1)
                                {
                                    codeWriter.WriteLine($"{variableAttributeFlagsStrList[i]},");
                                }
                                else
                                {
                                    codeWriter.WriteLine(variableAttributeFlagsStrList[i]);
                                }
                            }
                            codeWriter.WriteLine("),");
                        }
                    }
                    else
                    {
                        codeWriter.WriteLine("nox::reflection::VariableAttributeFlag::None,\t//\tvariable attribute flags");
                    }
                }

                //  getter, setterの記述
                //  reflection_generatedプロジェクトのsupport_functionsにあるマクロを使用する
                ReadOnlySpan<char> variableTypeDeclStr = $"using VariableType = decltype({variableInfo.FullName});";
             //   ReadOnlySpan<char> elementTypeDeclStr = "using ElementType = nox::ContainerElementType<VariableType>;";

                if (variableInfo.VariableAttributeFlags.IsOn(Parser2.VariableAttributeFlag.Static) == false)
                {
                    System.Diagnostics.Debug.Assert(declarationTypeInfo != null, $"declarationTypeInfo is null\t{variableInfo.FullName}");

                    //  メンバ
                    ReadOnlySpan<char> classTypeDeclStr = $"using ClassType = {declarationTypeInfo.FullName};";

                    //{
                    //	if (variableInfo.Type is Parser2.PointerTypeInfo pointerTypeInfo)
                    //	{
                    //		if (pointerTypeInfo.PointeeType.TypeAttributeFlags.IsOn(Parser2.TypeAttributeFlag.Const))
                    //		{
                    //			classTypeDeclStr = $"using ClassType = const {declarationTypeInfo.FullName};";
                    //			instanceImplStr = "ClassType instanceImpl = static_cast<ClassType>(instance);";

                    //		}
                    //		else
                    //		{
                    //			classTypeDeclStr = $"using ClassType = {declarationTypeInfo.FullName};";
                    //			instanceImplStr = "ClassType instanceImpl = static_cast<ClassType>(instance);";
                    //		}
                    //	}
                    //	else
                    //	{
                    //		classTypeDeclStr = $"using ClassType = {declarationTypeInfo.FullName};";
                    //	}
                    //}

                    // setter
                    {
                        codeWriter.WriteLine("//\tsetter");

                        if (noSetter)
                        {
							codeWriter.WriteLine("nullptr,");
						}
						else
						{
							codeWriter.WriteLine("+[](void* instance, void* value)->void");
							codeWriter.WriteLine("{");
							using (codeWriter.Indent())
							{
								codeWriter.WriteLine(variableTypeDeclStr);
								codeWriter.WriteLine(classTypeDeclStr);

								codeWriter.WriteLine("if constexpr(std::is_assignable_v<VariableType&, const std::remove_reference_t<VariableType>&>)");
								codeWriter.WriteLine("{");
								using (codeWriter.Indent())
								{
									codeWriter.WriteLine($"static_cast<ClassType*>(instance)->{variableInfo.FullName} = *static_cast <const std::remove_reference_t<VariableType>*> (value);");
								}
								codeWriter.WriteLine("}");
							}
							codeWriter.WriteLine("},");
						}
					}

                    // getter
                    {
                        codeWriter.WriteLine("//\tgetter");

                        if (noGetter)
                        {
							codeWriter.WriteLine("nullptr,");
						}
                        else
                        {
                            codeWriter.WriteLine($"+[](void* instance)->nox::reflection::ReflectionOptional<decltype({variableInfo.FullName})>");
                            codeWriter.WriteLine("{");
                            using (codeWriter.Indent())
                            {
                                codeWriter.WriteLine(variableTypeDeclStr);
                                codeWriter.WriteLine(classTypeDeclStr);

                                codeWriter.WriteLine("using OptionalValueType = typename nox::reflection::ReflectionOptional<VariableType>::value_type;");
                                codeWriter.WriteLine("if constexpr (std::is_copy_constructible_v<OptionalValueType>)");
                                codeWriter.WriteLine("{");
                                using (codeWriter.Indent())
                                {
                                    codeWriter.WriteLine($"return static_cast <const ClassType*> (instance)->{variableInfo.FullName};");
                                }
                                codeWriter.WriteLine("}");
                                codeWriter.WriteLine("else");
                                codeWriter.WriteLine("{");
                                using (codeWriter.Indent())
                                {
                                    codeWriter.WriteLine("return std::nullopt;");
                                }
                                codeWriter.WriteLine("}");
                            }
                            codeWriter.WriteLine("},");
                        }
                    }

                    // getter address
                    {
                        codeWriter.WriteLine("//\tgetter address");

                        if (noAddressOf)
                        {
                            codeWriter.WriteLine("nullptr,");
						}
                        else
                        {
                            codeWriter.WriteLine("+[](void* instance)->void*");
                            codeWriter.WriteLine("{");
                            using (codeWriter.Indent())
                            {
                                codeWriter.WriteLine(variableTypeDeclStr);
                                codeWriter.WriteLine(classTypeDeclStr);

                                codeWriter.WriteLine($"return const_cast<void*>(static_cast<const volatile void*>(std::addressof(static_cast<const ClassType*>(instance)->{variableInfo.FullName})));");
                            }
                            codeWriter.WriteLine("},");
                        }
                    }

                    // setter subscripts
                    {
                        codeWriter.WriteLine("//\tsetter subscripts");

                        codeWriter.WriteLine("nullptr,");
#if false
					codeWriter.WriteLine("+[](void* instance, const void* const value, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);
					codeWriter.WriteLine(classTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
                    codeWriter.WriteLine(elementTypeDeclStr);

                    codeWriter.WriteLine($"if (nox::util::IsValidIndex(static_cast<ClassType*>(instance)->{variableInfo.FullName}, index) == false)");
                    codeWriter.WriteLine("{");
                    codeWriter.Push();
                    codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
                    codeWriter.WriteLine("{");
                    codeWriter.Push();
                    codeWriter.WriteLine($"static_cast<ClassType*>(instance)->{variableInfo.FullName}[index] = *static_cast<ElementType*>(value);");
                    codeWriter.WriteLine("return true;");

					codeWriter.Pop();
                    codeWriter.WriteLine("}");

					codeWriter.Pop();

					codeWriter.WriteLine("}");
					codeWriter.WriteLine("else");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("},");
#endif
                    }

                    // getter subscripts
                    {
                        codeWriter.WriteLine("//\tgetter subscripts");

                        codeWriter.WriteLine("nullptr,");
#if false
					codeWriter.WriteLine("+[](void* out, const void* instance, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);
					codeWriter.WriteLine(classTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(elementTypeDeclStr);

					codeWriter.WriteLine($"if (nox::util::IsValidIndex(static_cast<ClassType*>(instance)->{variableInfo.FullName}, index) == false)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine($"*static_cast<ElementType*>(out) = static_cast<const ClassType*>(instance)->{variableInfo.FullName}[index];");
					codeWriter.WriteLine("return true;");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("else");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("},");
#endif
                    }

                    // getter address subscripts
                    {
                        codeWriter.WriteLine("//\tgetter subscripts");

                        codeWriter.WriteLine("nullptr");
#if false
					codeWriter.WriteLine("+[](void* out, void* instance, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);
					codeWriter.WriteLine(classTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(elementTypeDeclStr);

					codeWriter.WriteLine($"if (nox::util::IsValidIndex(static_cast<ClassType*>(instance)->{variableInfo.FullName}, index) == false)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
					codeWriter.Push();
					codeWriter.WriteLine("{");
					codeWriter.WriteLine($"*static_cast<std::add_pointer_t<nox::AddConstPointerType<ElementType*>>>(out) = &static_cast<ClassType*>(instance)->{variableInfo.FullName}[index];");
					codeWriter.WriteLine("return true;");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("else");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();

					codeWriter.WriteLine("}");
#endif
                    }
                }
                else
                {
                    //  グローバル

                    // setter
                    {
                        codeWriter.WriteLine("//\tsetter");

                        if (noSetter)
                        {
                            codeWriter.WriteLine("nullptr,");
                        }
                        else
                        {
                            codeWriter.WriteLine("+[](void* value)");
                            codeWriter.WriteLine("{");
                            using (codeWriter.Indent())
                            {
                                codeWriter.WriteLine(variableTypeDeclStr);

                                codeWriter.WriteLine("if constexpr (std::is_assignable_v<VariableType&, const std::remove_reference_t<VariableType>&>)");
                                codeWriter.WriteLine("{");
                                using (codeWriter.Indent())
                                {
                                    codeWriter.WriteLine($"{variableInfo.FullName} = *static_cast<const std::remove_reference_t<VariableType>*>(value);");
                                }
                                codeWriter.WriteLine("}");
                            }
                            codeWriter.WriteLine("},");
                        }
                    }

                    // getter
                    {
                        codeWriter.WriteLine("//\tgetter");

                        if (noGetter)
                        {
							codeWriter.WriteLine("nullptr,");
						}
                        else
                        {
                            codeWriter.WriteLine($"+[]()->nox::reflection::ReflectionOptional<decltype({variableInfo.FullName})>");
                            codeWriter.WriteLine("{");
                            using (codeWriter.Indent())
                            {
                                codeWriter.WriteLine(variableTypeDeclStr);

                                codeWriter.WriteLine("using OptionalValueType = typename nox::reflection::ReflectionOptional<VariableType>::value_type;");
                                codeWriter.WriteLine("if constexpr (std::is_copy_constructible_v<OptionalValueType>)");
                                codeWriter.WriteLine("{");
                                using (codeWriter.Indent())
                                {
                                    codeWriter.WriteLine($"return {variableInfo.FullName};");
                                }
                                codeWriter.WriteLine("}");
                                codeWriter.WriteLine("else");
                                codeWriter.WriteLine("{");
                                using (codeWriter.Indent())
                                {
                                    codeWriter.WriteLine("return std::nullopt;");
                                }
                                codeWriter.WriteLine("}");
                            }
                            codeWriter.WriteLine("},");
                        }
                    }

                    // getter address
                    {
                        codeWriter.WriteLine("//\tgetter address");

                        codeWriter.WriteLine("+[]()->void*");
                        codeWriter.WriteLine("{");
                        using (codeWriter.Indent())
                        {
                            codeWriter.WriteLine(variableTypeDeclStr);

                            codeWriter.WriteLine($"return const_cast<void*>(static_cast<const volatile void*>(std::addressof({variableInfo.FullName})));");
                        }
                        codeWriter.WriteLine("},");
                    }

                    // setter subscripts
                    {
                        codeWriter.WriteLine("//\tsetter subscripts");

                        codeWriter.WriteLine("nullptr,");

#if false
                    codeWriter.WriteLine("+[](const void* const value, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(elementTypeDeclStr);

					codeWriter.WriteLine($"if (nox::util::IsValidIndex(static_cast<ClassType*>(instance)->{variableInfo.FullName}, index) == false)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine($"{variableInfo.FullName}[index] = *static_cast<std::add_pointer_t<ElementType>>(value);");
					codeWriter.WriteLine("return true;");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("else");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("},");
#endif
                    }

                    // getter subscripts
                    {
                        codeWriter.WriteLine("//\tgetter subscripts");

                        codeWriter.WriteLine("nullptr,");
#if false
                    codeWriter.WriteLine("+[](void* out, const void* instance, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(elementTypeDeclStr);

					codeWriter.WriteLine($"if (nox::util::IsValidIndex({variableInfo.FullName}, index) == false)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine($"*static_cast<std::remove_reference_t<ElementType>*>(out) = {variableInfo.FullName}[index];");
					codeWriter.WriteLine("return true;");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("else");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("},");
#endif
                    }

                    // getter address subscripts
                    {
                        codeWriter.WriteLine("//\tgetter subscripts");

                        codeWriter.WriteLine("nullptr");
#if false
					codeWriter.WriteLine("+[](void* out, void* instance, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(elementTypeDeclStr);

					codeWriter.WriteLine($"if (nox::util::IsValidIndex({variableInfo.FullName}, index) == false)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
					codeWriter.Push();
					codeWriter.WriteLine("{");
					codeWriter.WriteLine($"*static_cast<std::add_pointer_t<nox::AddConstPointerType<ElementType*>>>(out) = &{variableInfo.FullName}[index];");
					codeWriter.WriteLine("return true;");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("}");

                    codeWriter.WriteLine("else");
                    codeWriter.WriteLine("{");
                    codeWriter.Push();
                    codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("}");
#endif
                    }
                }

            }
			codeWriter.WriteLine(");");

            return true;
		}

        private bool GenerateFunctionInfo(BaseCodeWriter codeWriter, Parser2.FunctionDecl functionInfo, Parser2.RecordDecl? declarationTypeInfo)
        {
			//  無名クラスはスキップ
			if (functionInfo.TypeInfo.TypeAttributeFlags.IsOn(Parser2.TypeAttributeFlag.Anonymous))
			{
                return false;
			}

			if (functionInfo.FullName.Contains("Typeof"))
			{
				Util.BreakPoint();
			}

			//TODO:  コンストラクタ、デストラクタは未対応
			if (functionInfo.FunctionAttributeFlags.IsAnyOn(
                 Parser2.FunctionAttributeFlag.DefaultConstructor |
                 Parser2.FunctionAttributeFlag.CopyConstructor |
                 Parser2.FunctionAttributeFlag.MoveConstructor |
                 Parser2.FunctionAttributeFlag.Destructor
                ))
            {
                return false;
            }

			//  型情報にauto型など、推論型が含まれている且つ、=deleteされている関数は型情報を取得できないためスキップ
            

			int enabledAttributeLength;
            {
                ReadOnlySpan<Parser2.AttributeDecl> attributeList = functionInfo.AttributeSpan;
                if (attributeList.Length > 0)
                {
                    enabledAttributeLength = GenerateAttributeInfoList(codeWriter, attributeList, functionInfo.Hash);
                }
                else
                {
                    enabledAttributeLength = 0;
                }
            }

            ReadOnlySpan<char> functionResultTypeDecl;
            if (declarationTypeInfo != null)
            {
                functionResultTypeDecl = $"decltype([]() -> decltype(auto) {{ return std::declval<{declarationTypeInfo.FullName}>().{functionInfo.FullName}(}})";

			}
            else
            {
				functionResultTypeDecl = $"decltype([]() -> decltype(auto) {{ return std::declval<>()->}})";
			}

            ReadOnlySpan<char> functionTypeFqn;
            bool isStatic = functionInfo.FunctionAttributeFlags.IsOn(Parser2.FunctionAttributeFlag.Static);
            if (isStatic)
            {
                functionTypeFqn = $"nox::ToFunctionPointerType<{functionInfo.TypeInfo.FullName}>";
            }
            else
            {
                Util.Assert(declarationTypeInfo != null);
                functionTypeFqn = $"nox::ToMemberFunctionPointerType<{functionInfo.TypeInfo.FullName}, {declarationTypeInfo.FullName}>";
            }

            ReadOnlySpan<Parser2.FunctionDecl.ArgumentInfo> argumentList = functionInfo.ArgumentSpan;
            int numArgument = argumentList.Length;
            //bool isNoexcept = functionInfo.FunctionAttributeFlags.IsOn(Parser2.FunctionAttributeFlag.Noexcept);

            //  純粋仮想関数や=delete関数は呼び出し不可なのでスキップ
            bool isCallableFunction = 
                functionInfo.FunctionAttributeFlags.IsOn(Parser2.FunctionAttributeFlag.Abstract) == false &&
                functionInfo.FunctionAttributeFlags.IsOn(Parser2.FunctionAttributeFlag.Delete) == false &&
                !functionInfo.FunctionAttributeFlags.IsOn(Parser2.FunctionAttributeFlag.Inline | Parser2.FunctionAttributeFlag.OutOfLine)
				;
			
				ReadOnlySpan<char> returnTypeFullName = functionInfo.TypeInfo.ReturnType.FullName;

                if (numArgument > 0)
                {
                    for (int i = 0, length = numArgument; i < length; ++i)
                    {
                        ref readonly Parser2.FunctionDecl.ArgumentInfo argumentInfo = ref argumentList[i];

                        codeWriter.WriteLine($"static constexpr nox::reflection::FunctionArgumentInfo function_arg_info_{functionInfo.Hash}_{i.ToString()} = nox::reflection::FunctionArgumentInfo(");
                        using (codeWriter.Indent())
                        {
                            codeWriter.WriteLine($"u8\"{argumentInfo.Name}\",\t//\tname");
                            codeWriter.WriteLine($"nullptr,\t//\tattribute_list");
                            codeWriter.WriteLine($"0,\t//\tattribute_length");
                            codeWriter.WriteLine($"nox::reflection::Typeof<{argumentInfo.TypeInfo.FullName}>(),\t//\ttype");
                            codeWriter.WriteLine($"{argumentInfo.IsDefault.ToLowerString()}\t//\tis_default");

                            codeWriter.WriteLine(");");
                        }
                    }

                    codeWriter.WriteLine($"static constexpr std::reference_wrapper<const nox::reflection::FunctionArgumentInfo> function_arg_table_{functionInfo.Hash}[{numArgument.ToString()}]={{");
                    using (codeWriter.Indent())
                    {

                        for (int i = 0, length = numArgument; i < length; ++i)
                        {
                            if (i != length - 1)
                            {
                                codeWriter.WriteLine($"function_arg_info_{functionInfo.Hash}_{i.ToString()},");
                            }
                            else
                            {
                                codeWriter.WriteLine($"function_arg_info_{functionInfo.Hash}_{i.ToString()}");
                            }
                        }

                        codeWriter.WriteLine("};");
                    }
                }
            if (isCallableFunction)
            {
                string preStr;
                {
                    if (isStatic == false || numArgument > 0)
                    {
                        if (functionInfo.FunctionAttributeFlags.IsOn(Parser2.FunctionAttributeFlag.Constexpr))
                        {
                            preStr = "+[](void** args) constexpr";
                        }
                        else
                        {
                            preStr = "+[](void** args)";
                        }
                    }
                    else
                    {
                        if (functionInfo.FunctionAttributeFlags.IsOn(Parser2.FunctionAttributeFlag.Constexpr))
                        {
                            preStr = "+[](void**) constexpr";
                        }
                        else
                        {
                            preStr = "+[](void**)";
                        }
                    }

						//    preStr += $" -> {returnTypeFullName} {{ ";
						//    preStr += $" -> decltype(auto) {{ ";
						preStr += $" -> nox::FunctionResultType<{functionTypeFqn}> {{ ";

					if (functionInfo.TypeInfo.ReturnType.TypeKind != Parser2.TypeKind.Void)
                    {
                        preStr += "return ";
                    }
                    //                    preStr += $"std::invoke(static_cast<std::remove_reference_t<{functionTypeFqn}>>(&{functionInfo.FullName})";

                    if (isStatic)
                    {
                        preStr += $"{functionInfo.FullName}(";
                    }
                    else
                    {
                        Util.Assert(declarationTypeInfo != null);
                        preStr += $"static_cast<std::add_pointer_t<{declarationTypeInfo.FullName}>>(args[0])->{functionInfo.FullName}(";
                    }
                }

                codeWriter.WriteLine($"static constexpr auto function_variation_table_{functionInfo.Hash} = std::array{{");
                using (codeWriter.Indent())
                {

                    for (int i = 0, length = 1 + (int)functionInfo.NumDefaultArgument; i < length; ++i)
                    {
                        string s = string.Empty;

                        int argLength = (int)numArgument - i;
                        for (int argIndex = 0; argIndex < argLength; ++argIndex)
                        {
                            ref readonly Parser2.FunctionDecl.ArgumentInfo argumentInfo = ref argumentList[argIndex];
                            int rawArgIndex = argIndex + (isStatic ? 0 : 1);

                            string tmp = $"std::forward_like<{argumentInfo.TypeInfo.FullName}>(*static_cast<std::add_pointer_t<std::remove_reference_t<{argumentInfo.TypeInfo.FullName}>>>(args[{rawArgIndex.ToString()}]))";

							if (argIndex == 0)
                            {
                                s += tmp;

							}
                            else
                            {
                                s += $", {tmp}";
                            }
                        }

                        if (i != length - 1)
                        {
                            codeWriter.WriteLine($"std::tuple{{ {preStr}{s}); }}, static_cast<std::uint8_t>({argLength.ToString()}) }},");
                            codeWriter.WriteNewLine();
                        }
                        else
                        {
                            codeWriter.WriteLine($"std::tuple{{ {preStr}{s}); }}, static_cast<std::uint8_t>({argLength.ToString()}) }}");
                        }
                    }
                }
                codeWriter.WriteLine("};");
                codeWriter.WriteNewLine();
            }

			codeWriter.WriteLine($"//\t{functionInfo.Meta.SourceLocation}");
			codeWriter.WriteLine($"static constexpr auto function_info_{functionInfo.Hash} = nox::reflection::detail::CreateFunctionInfo<{functionTypeFqn}>(");
            using (codeWriter.Indent())
            {
                //                codeWriter.WriteLine($"static_cast<std::remove_reference_t<{functionTypeFqn}>>(&{functionInfo.FullName}),\t//function_pointer");
                if (functionInfo.FunctionAttributeFlags.IsOn(Parser2.FunctionAttributeFlag.Delete))
                {
					codeWriter.WriteLine("nox::GetInvalidFunctionPointerId(),\t//\tfunction_id");
				}
				else
                {
					codeWriter.WriteLine($"nox::GetFunctionPointerId<static_cast<{functionTypeFqn}>(&{functionInfo.FullName})>(),\t//\tfunction_id");
				}

                codeWriter.WriteLine($"u8\"{functionInfo.Name}\",\t//\tname");
                codeWriter.WriteLine($"u8\"{functionInfo.FullName}\",\t//\tfullName");
                codeWriter.WriteLine($"u8\"{functionInfo.Namespace}\",\t//\tnamespace");

                if (declarationTypeInfo != null)
                {
                    codeWriter.WriteLine($"{functionInfo.AccessLevel.ToCppString()},\t//\taccess_level");
                }
                else
                {
                    codeWriter.WriteLine("nox::reflection::AccessLevel::Public,\t//\taccess_level");
                }

                if (enabledAttributeLength > 0)
                {
                    codeWriter.WriteLine($"attribute_info_table_{functionInfo.Hash},\t//\tattribute_info_table");
                }
                else
                {
                    codeWriter.WriteLine("nullptr,\t//\tattribute_info_table");

                }
                codeWriter.WriteLine($"{enabledAttributeLength},\t//\tattribute_info_length");

                if (numArgument > 0)
                {
                    codeWriter.WriteLine($"function_arg_table_{functionInfo.Hash},\t//\tfunction_arg_table");
                }
                else
                {
                    codeWriter.WriteLine("nullptr,\t//\tfunction_arg_table");
                }
                codeWriter.WriteLine($"{numArgument},\t//\targument_length");

                {
                    List<string> attributeStrList = new();

                    foreach (Parser2.FunctionAttributeFlag flag in EnumUtility<Parser2.FunctionAttributeFlag>.ValueList)
                    {
                        if (functionInfo.FunctionAttributeFlags.IsOn(flag) == false)
                        {
                            continue;
                        }

                        switch (flag)
                        {
                            case Parser2.FunctionAttributeFlag.Abstract:
                                attributeStrList.Add("nox::reflection::FunctionAttributeFlag::Abstract");
                                break;
                            case Parser2.FunctionAttributeFlag.Consteval:
                                attributeStrList.Add("nox::reflection::FunctionAttributeFlag::Consteval");
                                break;
                            case Parser2.FunctionAttributeFlag.Constexpr:
                                attributeStrList.Add("nox::reflection::FunctionAttributeFlag::Constexpr");
                                break;
                        }
                    }

                    int length = attributeStrList.Count;

                    if (length > 0)
                    {
                        codeWriter.WriteNest();
                        codeWriter.Write("nox::util::BitOr(");
                        for (int i = 0; i < length; ++i)
                        {
                            codeWriter.Write($"{attributeStrList[i]}");
                        }
                        codeWriter.Write(", nox::reflection::FunctionAttributeFlag::None)");
                        codeWriter.Write(",\t//\tfunction_attribute_flag");
                        codeWriter.WriteNewLine();
                    }
                    else
                    {
                        codeWriter.WriteLine("nox::reflection::FunctionAttributeFlag::None,\t//\tfunction_attribute_flag");
                    }
                }

                if (isCallableFunction)
                {
					codeWriter.WriteLine($"function_variation_table_{functionInfo.Hash}.data(),\t//\tfunction_holder_table");
					codeWriter.WriteLine($"{1 + functionInfo.NumDefaultArgument}\t//\tfunction_holder_length");
				}
                else
                {
					codeWriter.WriteLine("nullptr,\t//\tfunction_holder_table");
					codeWriter.WriteLine("0\t//\tfunction_holder_length");
				}
            }
            codeWriter.WriteLine(");");
            codeWriter.WriteNewLine();

			return true;
		}

        private bool GenerateEnumInfo(BaseCodeWriter codeWriter, Parser2.EnumDecl enumInfo, Parser2.RecordDecl? declarationTypeInfo)
        {
            ReadOnlySpan<Parser2.EnumDecl.EnumeratorInfo> enumeratorList = enumInfo.EnumeratorSpan;
            int numEnumerator = enumeratorList.Length;

			for (int i = 0, length = numEnumerator; i < length; ++i)
            {
                ref readonly var enumeratorInfo = ref enumeratorList[i];

                ReadOnlySpan<char> attribute_info_table_hash = $"{enumInfo.Hash}_{i.ToString()}";
                ReadOnlySpan<Parser2.AttributeDecl> enumeratorAttributeList = enumeratorInfo.AttributeSpan;

				int enabledAttributeLength;
                if (enumeratorAttributeList.Length > 0)
                {
                    enabledAttributeLength = GenerateAttributeInfoList(codeWriter, enumeratorAttributeList, attribute_info_table_hash);
                }
                else
                {
                    enabledAttributeLength = 0;
                }

                codeWriter.WriteLine($"static constexpr const auto enumerator_info_{enumInfo.Hash}_{i.ToString()} = nox::reflection::EnumeratorInfo(");
                using (codeWriter.Indent())
                {
                    if (enumeratorInfo.IsUnsigned == true)
                    {
                        codeWriter.WriteLine($"static_cast<std::uint64_t>({enumeratorInfo.Uint64.ToString()}),");
                    }
                    else
                    {
                        codeWriter.WriteLine($"static_cast<std::int64_t>({enumeratorInfo.Int64.ToString()}),");
                    }
                    codeWriter.WriteLine($"u8\"{enumeratorInfo.Name}\",\t//\tname");
                    codeWriter.WriteLine($"u8\"{enumInfo.FullName}::{enumeratorInfo.Name}\",\t//\tfullName");
                    if (enabledAttributeLength > 0)
                    {
                        codeWriter.WriteLine($"attribute_info_table_{attribute_info_table_hash},\t//\tattribute_info_table");
                    }
                    else
                    {
                        codeWriter.WriteLine($"nullptr,\t//\tattribute_info_table");
                    }
                    codeWriter.WriteLine($"{enabledAttributeLength}\t//\tattribute_length");
                }
                codeWriter.WriteLine(");");
            }

            {
                codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::EnumeratorInfo> enumerator_info_table_{enumInfo.Hash}[] = {{");
                using (codeWriter.Indent())
                {
                    for (int i = 0; i < numEnumerator; ++i)
                    {
                        if (i == numEnumerator - 1)
                        {
                            codeWriter.WriteLine($"enumerator_info_{enumInfo.Hash}_{i.ToString()}");
                        }
                        else
                        {
                            codeWriter.WriteLine($"enumerator_info_{enumInfo.Hash}_{i.ToString()},");
                        }
                    }
                }
                codeWriter.WriteLine("};");

				int enabledAttributeLength;

                {
                    ReadOnlySpan<Parser2.AttributeDecl> attributeList = enumInfo.AttributeSpan;

                    if (attributeList.Length > 0)
                    {
                        enabledAttributeLength = GenerateAttributeInfoList(codeWriter, attributeList, enumInfo.Hash);
                    }
                    else
                    {
                        enabledAttributeLength = 0;
                    }
                }

                codeWriter.WriteLine($"static constexpr const nox::reflection::EnumInfo enum_info_{enumInfo.Hash} = nox::reflection::EnumInfo(");
                using (codeWriter.Indent())
                {
                    codeWriter.WriteLine($"nox::reflection::Typeof<{enumInfo.FullName}>(),\t//\ttype");
                    codeWriter.WriteLine($"u8\"{enumInfo.Name}\",\t//\tname");
                    codeWriter.WriteLine($"u8\"{enumInfo.FullName}\",\t//\tfullName");
                    codeWriter.WriteLine($"u8\"{enumInfo.Namespace}\",\t//\tnamespace");
                    codeWriter.WriteLine($"{enumInfo.AccessLevel.ToCppString()},\t//\taccess_level");
                    if (enabledAttributeLength > 0)
                    {
                        codeWriter.WriteLine($"attribute_info_table_{enumInfo.Hash},\t//\tattribute_info_table");
                    }
                    else
                    {
                        codeWriter.WriteLine("nullptr,\t//\tattribute_info_table");
                    }
                    codeWriter.WriteLine($"{enabledAttributeLength.ToString()},\t//\tattribute_length");
                    codeWriter.WriteLine($"enumerator_info_table_{enumInfo.Hash},\t//\tenumerator_info_table");
                    codeWriter.WriteLine($"{numEnumerator.ToString()}\t//\tenumerator_length");
                }
                codeWriter.WriteLine(");");
                codeWriter.WriteNewLine();
            }

            return true;
        }

        private void GenerateClassInfo(BaseCodeWriter codeWriter, Parser2.RecordDecl classInfo, Parser2.RecordDecl rootDeclarationTypeInfo, List<string> registerDeclNameList, Parser2.RecordDecl? declarationTypeInfo)
        {
            //  template classはスキップ
            if (classInfo is Parser2.TemplateClassDecl)
            {
                return;
            }

            //  無名クラスはスキップ
            if (classInfo.RecordAttributeFlags.IsOn(Parser2.RecordAttributeFlag.Anonymous))
            {
                //TODO:  無名クラスのフィールドへのリフレクション対応
                return;
            }

			registerDeclNameList.Add($"nox::reflection::gen::ReflectionGeneratedHolder<{rootDeclarationTypeInfo.FullName}>::class_info_{classInfo.Hash}");
            codeWriter.WriteLine($"//\t{classInfo.Meta.SourceLocation}");

            //  内部クラスは、ReflectionGeneratedHolder化しない
            //  private reflectionの場合は要件等

            if (declarationTypeInfo == null)
            {
                codeWriter.WriteLine("template<>");
                codeWriter.WriteLine($"struct nox::reflection::gen::ReflectionGeneratedHolder<{classInfo.FullName}>");
                codeWriter.WriteLine("{");
            }
            else
            {
                codeWriter.WriteLineRegion(classInfo.FullName);
            }

            using (codeWriter.Indent())
            {

                //  クラス内クラス情報の生成
                int internalClassInfoListLength = 0;

                {
                    foreach (Parser2.RecordDecl child in classInfo.RecordList)
                    {
                        if (child.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
                        {
                            continue;
                        }

                        GenerateClassInfo(codeWriter, child, rootDeclarationTypeInfo, registerDeclNameList, classInfo);
                    }

                    int tmpInternalClassInfoListLength = classInfo.RecordList.Count;
                    if (tmpInternalClassInfoListLength > 0)
                    {
                        Span<int> enabledIndexList = stackalloc int[tmpInternalClassInfoListLength];
                        for (int i = 0; i < tmpInternalClassInfoListLength; ++i)
                        {
                            Parser2.RecordDecl type = classInfo.RecordList[i];
                            //  無名クラスはスキップ
                            if (type.RecordAttributeFlags.IsOn(Parser2.RecordAttributeFlag.Anonymous))
                            {
                                continue;
                            }

                            enabledIndexList[internalClassInfoListLength++] = i;

                            codeWriter.WriteLine($"static constexpr const nox::reflection::Type& internal_type_{classInfo.Hash}_{i.ToString()} = nox::reflection::Typeof<{type.FullName}>();");
                        }

                        if (internalClassInfoListLength > 0)
                        {
                            codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::Type> internal_type_table_{classInfo.Hash}[{internalClassInfoListLength.ToString()}] = {{");
                            using (codeWriter.Indent())
                            {

                                for (int i = 0; i < internalClassInfoListLength; ++i)
                                {
                                    int declIndex = enabledIndexList[i];
                                    if (i != internalClassInfoListLength - 1)
                                    {
                                        codeWriter.WriteLine($"internal_type_{classInfo.Hash}_{declIndex.ToString()},");
                                    }
                                    else
                                    {
                                        codeWriter.WriteLine($"internal_type_{classInfo.Hash}_{declIndex.ToString()}");
                                    }
                                }
                            }
                            codeWriter.WriteLine("};");
                        }
                    }
                }

                {
                    int baseTypeInfoListLength = classInfo.BaseSpan.Length;

                    if (baseTypeInfoListLength > 0)
                    {
                        codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::Type> base_type_table_{classInfo.Hash}[{baseTypeInfoListLength.ToString()}] = {{");
                        using (codeWriter.Indent())
                        {

                            for (int i = 0; i < baseTypeInfoListLength; ++i)
                            {
                                Parser2.BaseSpecifierDecl baseTypeInfo = classInfo.BaseSpan[i];
                                if (i != baseTypeInfoListLength - 1)
                                {
                                    codeWriter.WriteLine($"nox::reflection::Typeof<{baseTypeInfo.FullName}>(),");
                                }
                                else
                                {
                                    codeWriter.WriteLine($"nox::reflection::Typeof<{baseTypeInfo.FullName}>()");
                                }
                            }
                        }
                        codeWriter.WriteLine("};");
                    }
                }

                int enabledAttributeLength;
                {
                    ReadOnlySpan<Parser2.AttributeDecl> attributeList = classInfo.AttributeSpan;
                    if (attributeList.Length > 0)
                    {
                        enabledAttributeLength = GenerateAttributeInfoList(codeWriter, attributeList, classInfo.Hash);
                    }
                    else
                    {
                        enabledAttributeLength = 0;
                    }
                }

                List<string> variableInfoRegisterDeclNameList = new List<string>();
                {
                    foreach (Parser2.VariableDecl variableInfo in classInfo.VariableList)
                    {
						if (variableInfo.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
						{
							continue;
						}
						variableInfoRegisterDeclNameList.Add($"variable_info_{variableInfo.Hash}");
                        if (GenerateVariableInfo(codeWriter, variableInfo, classInfo) == true)
                        {
                            registerDeclNameList.Add($"nox::reflection::gen::ReflectionGeneratedHolder<{rootDeclarationTypeInfo.FullName}>::variable_info_{variableInfo.Hash}");
						}
					}

                    if (variableInfoRegisterDeclNameList.Count > 0)
                    {
                        codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::VariableInfo> variable_info_table_{classInfo.Hash}[{variableInfoRegisterDeclNameList.Count}] = {{");
                        using (codeWriter.Indent())
                        {
                            for (int i = 0, length = variableInfoRegisterDeclNameList.Count; i < length; ++i)
                            {
                                if (i != length - 1)
                                {
                                    codeWriter.WriteLine($"{variableInfoRegisterDeclNameList[i]},");
                                }
                                else
                                {
                                    codeWriter.WriteLine(variableInfoRegisterDeclNameList[i]);
                                }
                            }
                        }
                        codeWriter.WriteLine("};");
                    }
                }

                List<string> functionInfoRegisterDeclNameList = new List<string>();
                {
                    foreach (Parser2.FunctionDecl functionInfo in classInfo.FunctionList)
                    {
						if (functionInfo.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
						{
							continue;
						}
						functionInfoRegisterDeclNameList.Add($"function_info_{functionInfo.Hash}");
                        if (GenerateFunctionInfo(codeWriter, functionInfo, classInfo) == true)
                        {
                            registerDeclNameList.Add($"nox::reflection::gen::ReflectionGeneratedHolder<{rootDeclarationTypeInfo.FullName}>::function_info_{functionInfo.Hash}");
						}
                    }

                    if (functionInfoRegisterDeclNameList.Count > 0)
                    {
                        codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::FunctionInfo> function_info_table_{classInfo.Hash}[{functionInfoRegisterDeclNameList.Count}] = {{");
                        using (codeWriter.Indent())
                        {
                            for (int i = 0, length = functionInfoRegisterDeclNameList.Count; i < length; ++i)
                            {
                                if (i != length - 1)
                                {
                                    codeWriter.WriteLine($"{functionInfoRegisterDeclNameList[i]},");
                                }
                                else
                                {
                                    codeWriter.WriteLine(functionInfoRegisterDeclNameList[i]);
                                }
                            }
                        }
                        codeWriter.WriteLine("};");
                    }
                }

                List<string> enumInfoRegisterDeclNameList = new List<string>();

                {
                    foreach (Parser2.EnumDecl enumInfo in classInfo.EnumList)
                    {
						if (enumInfo.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
						{
							continue;
						}
						enumInfoRegisterDeclNameList.Add($"enum_info_{enumInfo.Hash}");
                        if (GenerateEnumInfo(codeWriter, enumInfo, classInfo) == true)
                        {
                            registerDeclNameList.Add($"nox::reflection::gen::ReflectionGeneratedHolder<{rootDeclarationTypeInfo.FullName}>::enum_info_{enumInfo.Hash}");
						}
                    }

                    int enumInfoRegisterDeclNameListLength = enumInfoRegisterDeclNameList.Count;
                    if (enumInfoRegisterDeclNameListLength > 0)
                    {
                        codeWriter.WriteLine($"static constexpr std::reference_wrapper<const nox::reflection::EnumInfo> enum_info_table_{classInfo.Hash}[{enumInfoRegisterDeclNameListLength.ToString()}] = {{");
                        using (codeWriter.Indent())
                        {
                            for (int i = 0, length = enumInfoRegisterDeclNameListLength; i < length; ++i)
                            {
                                if (i != length - 1)
                                {
                                    codeWriter.WriteLine($"{enumInfoRegisterDeclNameList[i]},");
                                }
                                else
                                {
                                    codeWriter.WriteLine(enumInfoRegisterDeclNameList[i]);
                                }
                            }
                        }
                        codeWriter.WriteLine("};");
                    }
                }

                codeWriter.WriteLine($"static constexpr const nox::reflection::ClassInfo class_info_{classInfo.Hash} = nox::reflection::ClassInfo(");
                using (codeWriter.Indent())
                {
                    codeWriter.WriteLine($"nox::reflection::Typeof<{classInfo.FullName}>(),\t//\ttype");
                    codeWriter.WriteLine($"u8\"{classInfo.Name}\",\t//\tname");
                    codeWriter.WriteLine($"u8\"{classInfo.FullName}\",\t//\tfullName");
                    codeWriter.WriteLine($"u8\"{classInfo.Namespace}\",\t//\tnamespace");
                    if (declarationTypeInfo != null)
                    {
                        codeWriter.WriteLine($"nox::reflection::Typeof<{declarationTypeInfo.FullName}>(),\t//\texternal_class_type");
                    }
                    else
                    {
                        codeWriter.WriteLine("nox::reflection::GetInvalidType(),\t//\texternal_class_type");
                    }
                    if (classInfo.BaseSpan.Length > 0)
                    {
                        codeWriter.WriteLine($"base_type_table_{classInfo.Hash},\t//\tbase_type_list");
                    }
                    else
                    {
                        codeWriter.WriteLine("nullptr,\t//\tbase_type_list");
                    }
                    codeWriter.WriteLine($"{classInfo.BaseSpan.Length.ToString()},\t//\tbase_type_length");

                    if (enabledAttributeLength > 0)
                    {
                        codeWriter.WriteLine($"attribute_info_table_{classInfo.Hash},\t//\tattribute_list");
                    }
                    else
                    {
                        codeWriter.WriteLine("nullptr,\t//\tattribute_list");
                    }
                    codeWriter.WriteLine($"{enabledAttributeLength.ToString()},\t//\tattribute_length");

                    if (variableInfoRegisterDeclNameList.Count > 0)
                    {
                        codeWriter.WriteLine($"variable_info_table_{classInfo.Hash},\t//\tvariable_info_list");
                    }
                    else
                    {
                        codeWriter.WriteLine("nullptr,\t//\tvariable_info_list");
                    }
                    codeWriter.WriteLine($"{variableInfoRegisterDeclNameList.Count.ToString()},\t//\tvariable_info_length");

                    if (functionInfoRegisterDeclNameList.Count > 0)
                    {
                        codeWriter.WriteLine($"function_info_table_{classInfo.Hash},\t//\tfunction_info_list");
                    }
                    else
                    {
                        codeWriter.WriteLine("nullptr,\t//\tfunction_info_list");
                    }
                    codeWriter.WriteLine($"{functionInfoRegisterDeclNameList.Count.ToString()},\t//\tfunction_info_length");
                    if (enumInfoRegisterDeclNameList.Count > 0)
                    {
                        codeWriter.WriteLine($"enum_info_table_{classInfo.Hash},\t//\tenum_info_list");
                    }
                    else
                    {
                        codeWriter.WriteLine("nullptr,\t//\tenum_info_list");
                    }
                    codeWriter.WriteLine($"{enumInfoRegisterDeclNameList.Count.ToString()},\t//\tenum_info_length");

                    if (internalClassInfoListLength > 0)
                    {
                        codeWriter.WriteLine($"internal_type_table_{classInfo.Hash},\t//\tinternal_type_list");
                    }
                    else
                    {
                        codeWriter.WriteLine("nullptr,\t//\tinternal_type_list");
                    }
                    codeWriter.WriteLine($"{internalClassInfoListLength.ToString()}\t//\tinternal_type_length");

                }
                codeWriter.WriteLine(");");
            }

            if (declarationTypeInfo == null)
            {
                codeWriter.WriteLine("};");
            }
            else
            {
                codeWriter.WriteLineEndRegion(classInfo.FullName);
            }
            codeWriter.WriteNewLine();
        }

#endregion

#endregion
	}
}
