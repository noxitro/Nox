using ClangSharp;
using ReflectionGenerator.Parser;
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

            /// <summary>
            /// 
            /// </summary>
            public required IReadOnlyList<string> IncludeHeaderList { get; init; } 

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

        #endregion

        #region 公開プロパティ
        /// <summary>
        /// Key: モジュール名 Value: モジュールごとの型情報リスト
        /// </summary>
        public required IReadOnlyDictionary<string, List<Info.NamespaceDeclInfo>> TypeInfoListWithArtifactNameDict { get; init; }

		/// <summary>
		/// 1ファイル内の定義数
		/// </summary>
		public uint NumOfDefinitionInFile { private get; init; } = 10;

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

            //  モジュールごとのディレクトリを作成
            //  ~/gen/BuildSpec/Platform/ModuleName
            string[] moduleDirectoryList = new string[ModuleInfoList.Length];

            for (int i = 0; i < moduleDirectoryList.Length; ++i)
            {
                moduleDirectoryList[i] = System.IO.Path.GetFullPath($"{_BaseDirectory}/{ModuleInfoList[i].ArtifactName}");
                if (Directory.Exists(moduleDirectoryList[i]) == false)
                {
                    Directory.CreateDirectory(moduleDirectoryList[i]);
                }
            }

			//  モジュールごとのヘッダファイルを生成
			//  ~/gen/BuildSpec/Platform/gen_BuildSpec_Platform_ModuleName.h
			for (int moduleListIndex = 0; moduleListIndex < ModuleInfoList.Length; ++moduleListIndex)
            {
                ref readonly ARTIFACT_INFO moduleInfo = ref ModuleInfoList[moduleListIndex];
                string artifactName = moduleInfo.ArtifactName;

                string genHeaderFilePath = System.IO.Path.GetFullPath($"{moduleDirectoryList[moduleListIndex]}/gen_{Platform}_{Configuration}_{artifactName}.h");

                //  ファイルが存在する場合、かつ再ビルドフラグが立っていない場合はスキップ
                if (File.Exists(genHeaderFilePath) == true && moduleInfo.Build == false)
                {
                   // continue;
                }

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

                        codeWriter.WriteLine($"void\tRegister_{Platform}_{Configuration}_{artifactName}_{ClassInfoStr}_{indexStr}();");
                        codeWriter.WriteLine($"void\tUnregister_{Platform}_{Configuration}_{artifactName}_{ClassInfoStr}_{indexStr}();");

                        codeWriter.WriteLine($"void\tRegister_{Platform}_{Configuration}_{artifactName}_GlobalDecl_{indexStr}();");
                        codeWriter.WriteLine($"void\tUnregister_{Platform}_{Configuration}_{artifactName}_GlobalDecl_{indexStr}();");
                    }

                    codeWriter.PopScope();

                    codeWriter.WriteLinePPEndIf(PlatformDefine);
                    codeWriter.WriteLinePPEndIf(ConfigurationDefine);
                }
            }

            //  モジュールごとのソースファイルを生成
            GenerateDeclaration();

			//  ~/gen/BuildSpec/Platform/gen_BuildSpec_Platform_ModuleName_{index}.cpp

            //  統括ソースファイルを生成
            {
                string unitySourceFilePath = System.IO.Path.GetFullPath($"{_BaseDirectory}/gen_{Platform}_{Configuration}.cpp");
                using (CodeWriter codeWriter = new CodeWriter(unitySourceFilePath))
                {
                    codeWriter.WriteLineSource();
                    codeWriter.WriteNewLine();

					codeWriter.WriteIncludeStdafx();
                    codeWriter.WriteLineInclude($"gen.h");

                    codeWriter.WriteNewLine();

                    codeWriter.WriteLinePPIf(ConfigurationDefine);
                    codeWriter.WriteLinePPIf(PlatformDefine);

                    //  インテリジェンス環境では無効にする
                    codeWriter.WriteLinePPIfNotDefine(CPP_DEFINE.INTELLISENSE);

                    //  各ヘッダファイルをインクルード
                    for (int i = 0; i < ModuleInfoList.Length; ++i)
                    {
                        ref readonly ARTIFACT_INFO moduleInfo = ref ModuleInfoList[i];
                        codeWriter.WriteLineInclude($"{moduleInfo.ArtifactName}/gen_{Platform}_{Configuration}_{moduleInfo.ArtifactName}.h");
                    }

					{
						// 登録
						codeWriter.WriteLine($"void ::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}()");
                        {
                            codeWriter.PushScope(CodeWriter.ScopeType.Define);
							for (int i = 0; i < ModuleInfoList.Length; ++i)
                            {
                                ref readonly ARTIFACT_INFO moduleInfo = ref ModuleInfoList[i];
                                codeWriter.WriteLineRegion(moduleInfo.ArtifactName);
                                for (int divideIndex = 0; divideIndex < NumOfDefinitionInFile; ++divideIndex)
                                {
                                    string indexStr = divideIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_{moduleInfo.ArtifactName}_{ClassInfoStr}_{indexStr}();");
                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_{moduleInfo.ArtifactName}_GlobalDecl_{indexStr}();");
                                }
                                codeWriter.WriteLineEndRegion(moduleInfo.ArtifactName);
                            }
							codeWriter.PopScope();
                        }

                        // 登録解除
                        codeWriter.WriteLine($"void ::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}()");
                        {
                            codeWriter.PushScope(CodeWriter.ScopeType.Define);
							for (int i = 0; i < ModuleInfoList.Length; ++i)
                            {
                                ref readonly ARTIFACT_INFO moduleInfo = ref ModuleInfoList[i];
                                codeWriter.WriteLineRegion(moduleInfo.ArtifactName);
                                for (int divideIndex = 0; divideIndex < NumOfDefinitionInFile; ++divideIndex)
                                {
                                    string indexStr = divideIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_{moduleInfo.ArtifactName}_{ClassInfoStr}_{indexStr}();");
                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_{moduleInfo.ArtifactName}_GlobalDecl_{indexStr}();");
                                }
                                codeWriter.WriteLineEndRegion(moduleInfo.ArtifactName);
                            }
							codeWriter.PopScope();
                        }
                    }

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

#if false
            //  
            string genSourceFilePath = $"{_BaseDirectory}/gen_{BuildSpec}_{Platform}.cpp";
            using (CodeWriter codeWriter = new CodeWriter(genHeaderFilePath))
            {
                codeWriter.WriteLineCopyRight();
                codeWriter.WriteLineSource();


                codeWriter.WriteLinePPIf(BuildSpecDefine);
                codeWriter.WriteLinePPIf(PlatformDefine);


                codeWriter.WriteIncludeStdafx();
                codeWriter.WriteLineInclude(genHeaderFilePath);

                codeWriter.WriteNamespace($"void\tnox::reflection::gen::Register{BuildSpec}{Platform}");
                codeWriter.PushScope(CodeWriter.ScopeType.Define);

                for (int i = 0; i < DivisionInfoCount; ++i)
                {
                    string indexStr = i.ToString().PadLeft(NUM_DIGIT_INDEX, '0');
                    codeWriter.WriteLine($"Register{BuildSpec}{Platform}_{UserDefinedCompoundTypeInfoStr}_{indexStr}();");
                    codeWriter.WriteLine($"Register{BuildSpec}{Platform}_{GlobalStr}_{indexStr}();");
                }

                codeWriter.PopScope();

                codeWriter.WriteNamespace($"void\tnox::reflection::gen::Unregister{BuildSpec}{Platform}");
                codeWriter.PushScope(CodeWriter.ScopeType.Define);

                for (int i = 0; i < DivisionInfoCount; ++i)
                {
                    string indexStr = i.ToString().PadLeft(2, '0');
                    codeWriter.WriteLine($"Unregister{BuildSpec}{Platform}_{UserDefinedCompoundTypeInfoStr}_{indexStr}();");
                    codeWriter.WriteLine($"Unregister{BuildSpec}{Platform}_{GlobalStr}_{indexStr}();");
                }

                codeWriter.PopScope();

                codeWriter.WriteLinePPEndIf(PlatformDefine);
                codeWriter.WriteLinePPEndIf(BuildSpecDefine);
            }

#endif

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

		private class JobSystem
        {
            private readonly List<Action> _JobList = new List<Action>();

            public void AddJob(Action job)
            {
                _JobList.Add(job);
            }

            public void Clear()
            {
                _JobList.Clear();
            }

			public void ExecuteParallel(int maxDegreeOfParallelism = 1)
            {
                System.Threading.Tasks.Parallel.ForEach(_JobList, new System.Threading.Tasks.ParallelOptions()
                {
                    MaxDegreeOfParallelism = maxDegreeOfParallelism
                }, job => job.Invoke());
			}
		}

        private void GenerateDeclaration()
        {
			foreach (var pair in TypeInfoListWithArtifactNameDict)
			{
                ReadOnlySpan<char> artifactName = pair.Key;

                var moduleInfo = Array.Find(ModuleInfoList, x => x.ArtifactName == pair.Key);

				//  書き込み対象のリスト
				CodeWriter[] codeWriterWithClassList = new CodeWriter[NumOfDefinitionInFile];
				CodeWriter[] codeWriterWithGlobalList = new CodeWriter[NumOfDefinitionInFile];

                //  ファイル数で分割する
                for (int i = 0; i < NumOfDefinitionInFile; ++i)
                {
                 
                    {
                        string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/{artifactName}/{artifactName}_{ClassInfoStr}_{i}.cpp");
                        codeWriterWithClassList[i] = new CodeWriter(path);
                    }

                    {
                        string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/{artifactName}/{artifactName}_GlobalDecl_{i}.cpp");
                        codeWriterWithGlobalList[i] = new CodeWriter(path);
                    }

                }

				int useThreadCount = Util.MAX_THREAD_ID;
				useThreadCount = 1;    //  デバッグのため、スレッド数を1に固定
				{
				   //MEMO 追加のインクルードディレクトリにプロジェクトディレクトリを指定している必要がある
					string baseHeaderFilePath = $"gen_{Platform}_{Configuration}_{artifactName}.h";

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
						codeWriter.WriteIncludeStdafx();
						codeWriter.WriteLine($"#include\t\"{baseHeaderFilePath}\"");

						codeWriter.WriteNewLine();

						//  プリプロセッサ
						codeWriter.WriteLinePPIf(ConfigurationDefine);
						codeWriter.WriteLinePPIf(PlatformDefine);
                        codeWriter.WriteLinePPIfNot(CPP_DEFINE.INTELLISENSE);

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

				GenerateWithArtifact(pair.Key, pair.Value, codeWriterWithGlobalList, codeWriterWithClassList);

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
					codeWriter.WriteLinePPEndIf(CPP_DEFINE.INTELLISENSE);
                    codeWriter.WriteLinePPEndIf(PlatformDefine);
                    codeWriter.WriteLinePPEndIf(ConfigurationDefine);
				},
                useThreadCount);
                

				foreach (CodeWriter codeWriter in codeWriterWithGlobalList)
                {
                    codeWriter.Dispose();
				}
                foreach(CodeWriter codeWriter in codeWriterWithClassList)
                {
                    codeWriter.Dispose();
				}
			}
		}

        private void GenerateWithArtifact(string artifactName, IReadOnlyList<Info.NamespaceDeclInfo> namespaceDeclInfoList, CodeWriter[] codeWriterWithGlobalList, CodeWriter[] codeWriterWithClassList)
        {
            int useThreadCount = Util.MAX_THREAD_ID;
            useThreadCount = 1;    //  デバッグのため、スレッド数を1に固定

			List<Info.VariableInfo> globalVariableInfoList = new List<Info.VariableInfo>();
            List<Info.FunctionInfo> globalFunctionInfoList = new List<Info.FunctionInfo>();
            List<Info.EnumInfo> globalEnumInfoList = new List<Info.EnumInfo>();
            List<Info.ClassInfo> classInfoList = new List<Info.ClassInfo>();

            System.Diagnostics.Stopwatch stopwatch = new System.Diagnostics.Stopwatch();
            stopwatch.Start();

            //  定義を収集
            {
                void probe(Info.NamespaceDeclInfo namespaceDeclInfo)
                {
                    globalVariableInfoList.AddRange(namespaceDeclInfo.VariableInfoList);
                    globalFunctionInfoList.AddRange(namespaceDeclInfo.FunctionInfoList);
                    globalEnumInfoList.AddRange(namespaceDeclInfo.EnumInfoList);
                    classInfoList.AddRange(namespaceDeclInfo.ClassInfoList);

                    foreach (var info in namespaceDeclInfo.DeclHolderList)
                    {
                        probe(info);
                    }
                }

                namespaceDeclInfoList.ForEach(probe);
            }

            Trace.InfoLine(this, $"setup Declarations {stopwatch.ElapsedMilliseconds.ToString()}msec");

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
                codeStringBuilder.Push();

				if (index < globalDeclarationLength)
                {
                    //  グローバル宣言
                    int globalDeclarationIndex = index;
                    string declName;
                    if (globalDeclarationIndex < globalVariableInfoList.Count)
                    {
                        //  変数宣言
                        Info.VariableInfo variableInfo = globalVariableInfoList[globalDeclarationIndex];
                        if(variableInfo.IsReflection == false)
                        {
                            return;
						} 
						GenerateVariableInfo(codeStringBuilder, variableInfo, null);

                        declName = $"variable_info_{variableInfo.Hash.ToString()}";

                    }
                    else if (globalDeclarationIndex < globalVariableInfoList.Count + globalFunctionInfoList.Count)
                    {
                        //  関数宣言
                        Info.FunctionInfo functionInfo = globalFunctionInfoList[globalDeclarationIndex - globalVariableInfoList.Count];
                        if(functionInfo.IsReflection == false)
                        {
                            return;
                        }

                        GenerateFunctionInfo(codeStringBuilder, functionInfo, null);
                        declName = $"function_info_{functionInfo.Hash.ToString()}";
                    }
                    else
                    {
                        //  Enum宣言
                        Info.EnumInfo enumInfo = globalEnumInfoList[globalDeclarationIndex - globalVariableInfoList.Count - globalFunctionInfoList.Count];
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
					CodeWriter codeWriter = codeWriterWithClassList[codeWriterWithClassIndex];
					Info.ClassInfo classInfo = classInfoList[classDeclarationIndex];

                    if(classInfo.Namespace.StartsWith("nox::") == false)
                    {
                        //  未対応
                        return;
                    }

                    List<string> registerClassDeclNameList = new List<string>();
					GenerateClassInfo(codeStringBuilder, classInfo, registerClassDeclNameList);


					lock (codeWriter)
                    {
                        codeWriter.WriteLine(codeStringBuilder.ToString());
                        registerDeclListTableWithClass[codeWriterWithClassIndex].AddRange(registerClassDeclNameList);
//						registerDeclListTableWithClass[codeWriterWithClassIndex].Add($"nox::reflection::gen::ReflectionGeneratedHolder<{classInfo.FullName}>::class_info_{classInfo.Hash}");
					}
                }

                codeStringBuilder.Pop();
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

						declStrRegister = $"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_{artifactName}_GlobalDecl_{indexStr}()";
						declStrUnregister =$"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_{artifactName}_GlobalDecl_{indexStr}()";
					}
                    else
                    {
						int classDeclarationIndex = index - codeWriterWithGlobalList.Length;
                        int codeWriterWithClassIndex = classDeclarationIndex;
						codeWriter = codeWriterWithClassList[codeWriterWithClassIndex];

                        declarationList = registerDeclListTableWithClass[codeWriterWithClassIndex];

						ReadOnlySpan<char> indexStr = codeWriterWithClassIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

						declStrRegister = $"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_{artifactName}_ClassDecl_{indexStr}()";
						declStrUnregister = $"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_{artifactName}_ClassDecl_{indexStr}()";
					}

					//  登録処理
					codeWriter.WriteLine(declStrRegister);
                    codeWriter.WriteLine("{");
					codeWriter.Push();

                    foreach (ReadOnlySpan<char> s in declarationList)
                    {
                        codeWriter.WriteLine($"nox::reflection::Register({s});");
                    }

                    codeWriter.Pop();
                    codeWriter.WriteLine("}");

                    codeWriter.WriteNewLine();

					//  登録解除処理
					codeWriter.WriteLine(declStrUnregister);
					codeWriter.WriteLine("{");
                    codeWriter.Push();

					foreach (ReadOnlySpan<char> s in declarationList)
                    {
                        codeWriter.WriteLine($"nox::reflection::Unregister({s});");
                    }

					codeWriter.Pop();
					codeWriter.WriteLine("}");
				},
                useThreadCount);
        }

        private static string GetVariableInfoDeclName(Info.VariableInfo variableInfo) => $"variable_info_{variableInfo.Hash}";

        #region 定義生成群
        private int GenerateAttributeInfoList(BaseCodeWriter codeWriter, IReadOnlyList<Info.AttributeInfo> attributeInfoList, ReadOnlySpan<char> hash)
        {
            List<string> tmpDeclNameList = new List<string>();
			foreach (Info.AttributeInfo attributeInfo in attributeInfoList)
			{
                string attr_decl_name = $"attribute_info_{attributeInfo.Hash}";
				tmpDeclNameList.Add(attr_decl_name);
				switch (attributeInfo)
				{
					case Info.EngineAnnotateAttribute engineAnnotateAttribute:

						string fullDecl = engineAnnotateAttribute.Value;
						codeWriter.WriteLine($"static constexpr decltype(auto) {attr_decl_name} = {fullDecl};");
						break;

					default:
                        ReadOnlySpan<char> standardAttrKindName = attributeInfo.AttrKind switch
                        {
                            ClangSharp.Interop.CX_AttrKind.CX_AttrKind_NoReturn => "nox::reflection::StandardAttrKind::NoDiscard",
                            _ => "nox::reflection::StandardAttrKind::Invalid"
						};

						codeWriter.WriteLine($"static constexpr nox::reflection::attr::StandardAttribute {attr_decl_name} = nox::reflection::attr::StandardAttribute({standardAttrKindName});");
						break;
				}
			}

            codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::ReflectionObject> attribute_info_table_{hash}[{attributeInfoList.Count.ToString()}]={{");
            codeWriter.Push();
			for (int i = 0, length = tmpDeclNameList.Count; i < length; ++i)
            {
                string declName = tmpDeclNameList[i];
                if(i == length - 1)
                {
					codeWriter.WriteLine(declName);
				}
                else
                {
                    codeWriter.WriteLine($"{declName},");
				}
            }
            codeWriter.Pop();
			codeWriter.WriteLine("};");
            codeWriter.WriteNewLine();

            return tmpDeclNameList.Count;
		}

        private void GenerateVariableInfo(BaseCodeWriter codeWriter, Info.VariableInfo variableInfo, Info.ClassInfo? declarationTypeInfo)
        {
			ref readonly var typeData = ref variableInfo.GetTypeData();

			//  属性定義
			int enabledAttributeLength;
			if (variableInfo.AttributeInfoList.Count > 0)
			{
				enabledAttributeLength = GenerateAttributeInfoList(codeWriter, variableInfo.AttributeInfoList, variableInfo.Hash);
			}
			else
			{
				enabledAttributeLength = 0;
			}

			//  メンバ変数
			if (variableInfo.IsStatic == false)
			{
				if (typeData.RefQualifierKind == ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_None)
				{
					codeWriter.WriteLine($"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoMember<&{variableInfo.FullName}>");
				}
				else
				{
					codeWriter.WriteLine($"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoMemberRef");
				}
			}
			//  グローバル変数
			else
			{
				if (typeData.RefQualifierKind == ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_None)
				{
					codeWriter.WriteLine($"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoGlobal<&{variableInfo.FullName}>");
				}
				else
				{
					codeWriter.WriteLine($"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoGlobalRef");
				}
			}
           
            codeWriter.WriteLine("(");
            codeWriter.Push();

			//  非参照型の場合
			if (typeData.RefQualifierKind == ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_None)
			{
				//  オブジェクトポインタ
				//	buffer += $"\t&{variableInfo.FullName},\t//\t object_pointer\n";
			}
			else
			{
				codeWriter.WriteLine($"nox::reflection::Typeof<decltype({variableInfo.FullName})>(),\t//\ttype");
				if (variableInfo.IsStatic == false)
				{
                    System.Diagnostics.Debug.Assert(declarationTypeInfo != null, "declarationTypeInfo must not be null for non-static variables.");
					codeWriter.WriteLine($"\tnox::reflection::Typeof<decltype({declarationTypeInfo.FullName})>(),\t//\towner type");
				}
			}

			//  各種名前
			codeWriter.WriteLine($"U\"{variableInfo.Name}\",\t//\tname");
			codeWriter.WriteLine($"U\"{variableInfo.FullName}\",\t//\tfullName");
			codeWriter.WriteLine($"U\"{variableInfo.Namespace}\",\t//\tnamespace");

			codeWriter.WriteLine($"{variableInfo.AccessLevel.GetRuntimeFqn()},\t//\taccess level");
			codeWriter.WriteLine($"{variableInfo.BitWith.ToString()},\t//\tbit with");
			codeWriter.WriteLine($"{variableInfo.Offset.ToString()},\t//\toffset");

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
				if (variableInfo.IsConstexpr == true)
				{
					variableAttributeFlagsStrList.Add("nox::reflection::VariableAttributeFlag::Constexpr");
				}
				if (variableInfo.IsStatic == true)
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
			ReadOnlySpan<char> elementTypeDeclStr = "using ElementType = nox::ContainerElementType<VariableType>;";

			if (variableInfo.IsStatic == false)
			{
                System.Diagnostics.Debug.Assert(declarationTypeInfo != null, $"declarationTypeInfo is null\t{variableInfo.FullName}");

                //  メンバ
           
                ReadOnlySpan<char> classTypeDeclStr = $"using ClassType = {declarationTypeInfo.FullName};";

				// setter
				{
                    codeWriter.WriteLine("//\tsetter");

                    if (typeData.IsConst == true)
                    {
                        codeWriter.WriteLine("nullptr,");
                    }
                    else
                    {
                        codeWriter.WriteLine("+[](nox::not_null<void*> instance, const void* const value)");
                        codeWriter.WriteLine("{");
                        codeWriter.Push();
                        codeWriter.WriteLine(variableTypeDeclStr);
                        codeWriter.WriteLine(classTypeDeclStr);

                        codeWriter.WriteLine("if constexpr(nox::concepts::Assignable<VariableType, VariableType>)");
                        codeWriter.WriteLine("{");
                        codeWriter.Push();
                        codeWriter.WriteLine($"static_cast<ClassType*>(instance.get())->{variableInfo.FullName} = *static_cast <const std::remove_reference_t<VariableType>*> (value);");
                        codeWriter.Pop();
                        codeWriter.WriteLine("}");

                        codeWriter.Pop();
                        codeWriter.WriteLine("},");
                    }
                }

				// getter
				{
					codeWriter.WriteLine("//\tgetter");

					codeWriter.WriteLine("+[](nox::not_null<void*> out, nox::not_null<const void*> instance)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);
					codeWriter.WriteLine(classTypeDeclStr);

					codeWriter.WriteLine("if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine($"*static_cast<std::remove_const_t<std::decay_t<VariableType>>*>(out.get()) = static_cast <const ClassType*> (instance.get())->{variableInfo.FullName};");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("},");
				}

				// getter address
				{
					codeWriter.WriteLine("//\tgetter address");

					codeWriter.WriteLine("+[](nox::not_null<void*> out, nox::not_null<void*> instance)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);
					codeWriter.WriteLine(classTypeDeclStr);

					codeWriter.WriteLine("if constexpr(std::convertible_to<VariableType, std::remove_const_t<VariableType>>)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine($"*static_cast<std::remove_reference_t<VariableType>**>(out.get()) = &static_cast<ClassType*>(instance.get())->{variableInfo.FullName};");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("},");
				}

				// setter subscripts
				{
					codeWriter.WriteLine("//\tsetter subscripts");

					codeWriter.WriteLine("nullptr,");
#if false
					codeWriter.WriteLine("+[](nox::not_null<void*> instance, const void* const value, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);
					codeWriter.WriteLine(classTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
                    codeWriter.WriteLine(elementTypeDeclStr);

                    codeWriter.WriteLine($"if (nox::util::IsValidIndex(static_cast<ClassType*>(instance.get())->{variableInfo.FullName}, index) == false)");
                    codeWriter.WriteLine("{");
                    codeWriter.Push();
                    codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
                    codeWriter.WriteLine("{");
                    codeWriter.Push();
                    codeWriter.WriteLine($"static_cast<ClassType*>(instance.get())->{variableInfo.FullName}[index] = *static_cast<ElementType*>(value);");
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
					codeWriter.WriteLine("+[](nox::not_null<void*> out, nox::not_null<const void*> instance, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);
					codeWriter.WriteLine(classTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(elementTypeDeclStr);

					codeWriter.WriteLine($"if (nox::util::IsValidIndex(static_cast<ClassType*>(instance.get())->{variableInfo.FullName}, index) == false)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine($"*static_cast<ElementType*>(out.get()) = static_cast<const ClassType*>(instance.get())->{variableInfo.FullName}[index];");
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
					codeWriter.WriteLine("+[](nox::not_null<void*> out, nox::not_null<void*> instance, const std::uint32_t index)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);
					codeWriter.WriteLine(classTypeDeclStr);

					codeWriter.WriteLine("if constexpr (nox::HasIndexOperatorValue<VariableType> == true)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(elementTypeDeclStr);

					codeWriter.WriteLine($"if (nox::util::IsValidIndex(static_cast<ClassType*>(instance.get())->{variableInfo.FullName}, index) == false)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine("return false;");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<ElementType, ElementType>)");
					codeWriter.Push();
					codeWriter.WriteLine("{");
					codeWriter.WriteLine($"*static_cast<std::add_pointer_t<nox::AddConstPointerType<ElementType*>>>(out.get()) = &static_cast<ClassType*>(instance.get())->{variableInfo.FullName}[index];");
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

					if (typeData.IsConst == true)
					{
						codeWriter.WriteLine("nullptr,");
					}
					else
					{
						codeWriter.WriteLine("+[](const void* const value)");
						codeWriter.WriteLine("{");
						codeWriter.Push();
						codeWriter.WriteLine(variableTypeDeclStr);

						codeWriter.WriteLine("if constexpr (nox::concepts::Assignable<VariableType, VariableType>)");
						codeWriter.WriteLine("{");
						codeWriter.Push();
						codeWriter.WriteLine($"{variableInfo.FullName} = *static_cast<const std::remove_reference_t<VariableType>*>(value);");
						codeWriter.Pop();
						codeWriter.WriteLine("}");

						codeWriter.Pop();
						codeWriter.WriteLine("},");
					}
				}

                // getter
                {
					codeWriter.WriteLine("//\tgetter");

					codeWriter.WriteLine("+[](nox::not_null<void*> out)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);

					codeWriter.WriteLine("if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine($"*static_cast<std::remove_const_t<std::decay_t<VariableType>>*>(out.get()) = {variableInfo.FullName};");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
					codeWriter.WriteLine("},");
				}

                // getter address
                {
					codeWriter.WriteLine("//\tgetter address");

					codeWriter.WriteLine("+[](nox::not_null<void*> instance, const void* const value)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine(variableTypeDeclStr);

					codeWriter.WriteLine("if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>)");
					codeWriter.WriteLine("{");
					codeWriter.Push();
					codeWriter.WriteLine($"*static_cast<std::remove_reference_t<VariableType>**>(out.get()) = &{variableInfo.FullName};");
					codeWriter.Pop();
					codeWriter.WriteLine("}");

					codeWriter.Pop();
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

					codeWriter.WriteLine($"if (nox::util::IsValidIndex(static_cast<ClassType*>(instance.get())->{variableInfo.FullName}, index) == false)");
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
                    codeWriter.WriteLine("+[](nox::not_null<void*> out, nox::not_null<const void*> instance, const std::uint32_t index)");
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
					codeWriter.WriteLine($"*static_cast<std::decay_t<ElementType>*>(out.get()) = {variableInfo.FullName}[index];");
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
					codeWriter.WriteLine("+[](nox::not_null<void*> out, nox::not_null<void*> instance, const std::uint32_t index)");
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
					codeWriter.WriteLine($"*static_cast<std::add_pointer_t<nox::AddConstPointerType<ElementType*>>>(out.get()) = &{variableInfo.FullName}[index];");
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

			codeWriter.Pop();
			codeWriter.WriteLine(");");
		}

        private bool GenerateFunctionInfo(BaseCodeWriter codeWriter, Info.FunctionInfo functionInfo, Info.ClassInfo? declarationTypeInfo)
        {
            //TODO:  コンストラクタ、デストラクタは未対応
            if(functionInfo.IsConstexpr || functionInfo.IsDestructor)
            {
                return false;
            }

			int enabledAttributeLength;
			if (functionInfo.AttributeInfoList.Count > 0)
			{
				enabledAttributeLength = GenerateAttributeInfoList(codeWriter, functionInfo.AttributeInfoList, functionInfo.Hash);
			}
			else
			{
				enabledAttributeLength = 0;
			}

			{
                int numArgument = functionInfo.ArgumentInfoList.Length;

				if (numArgument > 0)
                {
                    for (int i = 0, length = numArgument; i < length; ++i)
                    {
                        ref readonly Info.FunctionInfo.ArgumentInfo argumentInfo = ref functionInfo.ArgumentInfoList[i];


						codeWriter.WriteLine($"static constexpr auto function_arg_info_{functionInfo.Hash}_{i.ToString()} = nox::reflection::FunctionArgumentInfo(");
                        codeWriter.Push();

                        codeWriter.WriteLine($"U\"{argumentInfo.Name}\",\t//\tname");
                        codeWriter.WriteLine($"nullptr,");
                        codeWriter.WriteLine($"0,");
                        codeWriter.WriteLine($"nox::reflection::Typeof<{argumentInfo.TypeFullName}>(),");
                        codeWriter.WriteLine($"{argumentInfo.IsDefault.ToLowerString()}");

						codeWriter.WriteLine(");");
						codeWriter.Pop();
                    }

                    codeWriter.WriteLine($"static constexpr std::reference_wrapper<const nox::reflection::FunctionArgumentInfo> function_arg_table_{functionInfo.Hash}[{numArgument.ToString()}]={{");
                    codeWriter.Push();

                    for (int i = 0, length = numArgument; i < length; ++i)
                    {
                        if(i != length - 1)
                        {
                            codeWriter.WriteLine($"function_arg_info_{functionInfo.Hash}_{i.ToString()},");
                        }
                        else
                        {
                            codeWriter.WriteLine($"function_arg_info_{functionInfo.Hash}_{i.ToString()}");
						}   
                    }

					codeWriter.WriteLine("};");
					codeWriter.Pop();
				}
                
				codeWriter.WriteLine($"static constexpr auto function_info_{functionInfo.Hash} = nox::reflection::detail::CreateFunctionInfo(");
            }
            codeWriter.Push();
            
            codeWriter.WriteLine($"static_cast<std::decay_t<{functionInfo.FunctionTypeFullName}>>(&{functionInfo.FullName}),\t//function_pointer");
            codeWriter.WriteLine($"U\"{functionInfo.Name}\",\t//\tname");
            codeWriter.WriteLine($"U\"{functionInfo.FullName}\",\t//\tfullName");
            codeWriter.WriteLine($"U\"{functionInfo.Namespace}\",\t//\tnamespace");

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
            if (functionInfo.NumArguments > 0)
            {
                codeWriter.WriteLine($"function_arg_table_{functionInfo.Hash},\t//\tfunction_arg_table");
            }
            else
            {
                codeWriter.WriteLine("nullptr,\t//\tfunction_arg_table");
			}
            codeWriter.WriteLine($"{functionInfo.NumArguments},\t//\targument_length");
            codeWriter.WriteLine("nox::reflection::FunctionAttributeFlag::None,");

            int numArguments = (int)functionInfo.NumDefaultArguments;
            if(functionInfo.FullName == "nox::attr::Attribute::StaticAssertNoxDeclareReflectionObject")
            {
                Util.BreakPoint();
            }
			for (int i = 0, length = 1 + (int)functionInfo.NumDefaultArguments; i < length; ++i)
            {
                codeWriter.WriteNest();
				codeWriter.Write($"+[](");

				for (int argIndex = 0, argLength = (int)functionInfo.NumArguments - i; argIndex < argLength; ++argIndex)
				{
                    ref readonly Info.FunctionInfo.ArgumentInfo argumentInfo = ref functionInfo.ArgumentInfoList[argIndex];

					codeWriter.Write($"{argumentInfo.TypeFullName} arg{argIndex.ToString()}");
					if (argIndex != argLength - 1)
                    {
                        codeWriter.Write(",");
					}
				}

                if (functionInfo.IsConstexpr == true)
                {
                    codeWriter.Write($")constexpr noexcept({functionInfo.IsNoexcept.ToLowerString()})");
				}
                else
                {
					codeWriter.Write($")noexcept({functionInfo.IsNoexcept.ToLowerString()})");
				}
                codeWriter.WriteNewLine();
				codeWriter.WriteLine("{");
                codeWriter.Push();

                codeWriter.WriteNest();
				if (functionInfo.IsNoReturn == false)
				{
					codeWriter.Write("return ");
				}
                codeWriter.Write($"std::invoke(static_cast<std::decay_t<{functionInfo.FunctionTypeFullName}>>(&{functionInfo.FullName})");

                for (int argIndex = 0, argLength = (int)functionInfo.NumArguments - i; argIndex < argLength; ++argIndex)
                {
                    ref readonly Info.FunctionInfo.ArgumentInfo argumentInfo = ref functionInfo.ArgumentInfoList[argIndex];

                    codeWriter.Write($", arg{argIndex.ToString()}");
                }

				codeWriter.Write(");");
				codeWriter.WriteNewLine();

				codeWriter.Pop();
				codeWriter.WriteLine("}");

				codeWriter.Push();

                if (i != length - 1)
                {
					codeWriter.WriteLine(",");
				}
                codeWriter.Pop();
			}

			codeWriter.Pop();
            codeWriter.WriteLine(");");
            codeWriter.WriteNewLine();

			return true;
		}

        private void GenerateEnumInfo(BaseCodeWriter codeWriter, Info.EnumInfo enumInfo, Info.ClassInfo? declarationTypeInfo)
        {
            for (int i = 0, length = enumInfo.VariableList.Length; i < length; ++i)
            {
                ref readonly var enumeratorInfo = ref enumInfo.VariableList[i];

                ReadOnlySpan<char> attribute_info_table_hash = $"{enumInfo.Hash}_{i.ToString()}";

                int enabledAttributeLength;
                if (enumeratorInfo.AttributeInfoList.Count > 0)
                {
                    enabledAttributeLength = GenerateAttributeInfoList(codeWriter, enumeratorInfo.AttributeInfoList, attribute_info_table_hash);
                }
                else
                {
                    enabledAttributeLength = 0;
                }

                codeWriter.WriteLine($"static constexpr const auto enumerator_info_{enumInfo.Hash}_{i.ToString()} = nox::reflection::EnumeratorInfo(");
                codeWriter.Push();
                if (enumeratorInfo.IsUnsigned == true)
                {
                    codeWriter.WriteLine($"static_cast<std::uint64_t>({enumeratorInfo.Integer64.UInt64.ToString()}),");
                }
                else
                {
                    codeWriter.WriteLine($"static_cast<std::int64_t>({enumeratorInfo.Integer64.Int64.ToString()}),");
                }
                codeWriter.WriteLine($"U\"{enumeratorInfo.Name}\",\t//\tname");
                codeWriter.WriteLine($"U\"{enumInfo.FullName}::{enumeratorInfo.Name}\",\t//\tfullName");
                if (enabledAttributeLength > 0)
                {
                    codeWriter.WriteLine($"attribute_info_table_{attribute_info_table_hash},\t//\tattribute_info_table");
                }
                else
                {
                    codeWriter.WriteLine($"nullptr,\t//\tattribute_info_table");
                }
                codeWriter.WriteLine($"{enabledAttributeLength},\t//\tattribute_length");
                codeWriter.Pop();
                codeWriter.WriteLine("}};");
            }

            {
                codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::EnumeratorInfo> enumerator_info_table = {{");
                codeWriter.Push();
                for (int i = 0, length = enumInfo.VariableList.Length; i < length; ++i)
                {
                    codeWriter.WriteLine($"enumerator_info_{enumInfo.Hash}_{i.ToString()},");
                }
                codeWriter.Pop();
                codeWriter.WriteLine("};");

                int enabledAttributeLength;
                if (enumInfo.AttributeInfoList.Count > 0)
                {
                    enabledAttributeLength = GenerateAttributeInfoList(codeWriter, enumInfo.AttributeInfoList, enumInfo.Hash);
                }
                else
                {
                    enabledAttributeLength = 0;
                }

                codeWriter.WriteLine($"static constexpr const nox::reflection::EnumInfo enum_info_{enumInfo.Hash} = nox::reflection::EnumInfo(");
                codeWriter.Push();
                codeWriter.WriteLine($"nox::reflection::Typeof<{enumInfo.FullName}>(),\t//\ttype");
                codeWriter.WriteLine($"U\"{enumInfo.Name}\",\t//\tname");
                codeWriter.WriteLine($"U\"{enumInfo.FullName}\",\t//\tfullName");
                codeWriter.WriteLine($"U\"{enumInfo.Namespace}\",\t//\tnamespace");
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
                codeWriter.Pop();
                codeWriter.WriteLine("};");
                codeWriter.WriteNewLine();
            }
        }

        private void GenerateClassInfo(BaseCodeWriter codeWriter, Info.ClassInfo classInfo, List<string> registerDeclNameList, ReadOnlySpan<char> parentDeclName = default)
        {
            if(
                (classInfo.IsReflectionObject == false && classInfo.IsReflection == false) ||
                classInfo.IsIgnoreReflection == true
                )
            {
                return;
            }

            if (parentDeclName.IsEmpty)
            {
                registerDeclNameList.Add($"nox::reflection::gen::ReflectionGeneratedHolder<{classInfo.FullName}>::class_info_{classInfo.Hash}");
            }
            else
            {
				registerDeclNameList.Add($"{parentDeclName}::ReflectionGeneratedHolder<{classInfo.FullName}>::class_info_{classInfo.Hash}");
			}

            codeWriter.WriteLine("template<>");
            codeWriter.WriteLine($"struct nox::reflection::gen::ReflectionGeneratedHolder<{classInfo.FullName}>");
            codeWriter.WriteLine("{");
			codeWriter.Push();

            //  子クラス情報の生成
            {
                int internalClassInfoListLength = classInfo.ClassInfoList.Count;

                ReadOnlySpan<char> lastParentDeclName = registerDeclNameList.Last();

				foreach (Info.ClassInfo child in classInfo.ClassInfoList)
                {
                    GenerateClassInfo(codeWriter, child, registerDeclNameList, lastParentDeclName);
                }

                if (internalClassInfoListLength > 0)
                {
                    for (int i = 0; i < internalClassInfoListLength; ++i)
                    {
                        Info.ClassInfo type = classInfo.ClassInfoList[i];
                        codeWriter.WriteLine($"static constexpr const nox::reflection::Type& internal_type_{classInfo.Hash}_{i.ToString()} = nox::reflection::Typeof<decltype({type.FullName})>();");
                    }

					codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::Type> internal_type_table_{classInfo.Hash}[] = {{");
					codeWriter.Push();

					for (int i = 0; i < internalClassInfoListLength; ++i)
					{
						if (i != internalClassInfoListLength - 1)
						{
							codeWriter.WriteLine($"internal_type_{classInfo.Hash}_{i.ToString()},");
						}
						else
						{
							codeWriter.WriteLine($"internal_type_{classInfo.Hash}_{i.ToString()}");
						}
					}
					codeWriter.Pop();
					codeWriter.WriteLine("};");
				}
            }

			{
				int baseTypeInfoListLength = classInfo.BaseTypeInfoList.Count;

                if (baseTypeInfoListLength > 0)
                {
                    for (int i = 0; i < baseTypeInfoListLength; ++i)
                    {
                        Info.TypeInfo baseTypeInfo = classInfo.BaseTypeInfoList[i];
                        codeWriter.WriteLine($"static constexpr const nox::reflection::Type& base_type_{classInfo.Hash}_{i.ToString()} = nox::reflection::Typeof<decltype({baseTypeInfo.FullName})>();");
                    }

                    codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::Type> base_type_table_{classInfo.Hash}[] = {{");
                    codeWriter.Push();

                    for (int i = 0; i < baseTypeInfoListLength; ++i)
                    {
						if (i != baseTypeInfoListLength - 1)
						{
							codeWriter.WriteLine($"base_type_{classInfo.Hash}_{i.ToString()},");
						}
						else
						{
                            codeWriter.WriteLine($"base_type_{classInfo.Hash}_{i.ToString()}");
						}
                    }
                    codeWriter.Pop();
                    codeWriter.WriteLine("};");
                }
            }

			int enabledAttributeLength;
			if (classInfo.AttributeInfoList.Count > 0)
			{
				enabledAttributeLength = GenerateAttributeInfoList(codeWriter, classInfo.AttributeInfoList, classInfo.Hash);
			}
			else
			{
				enabledAttributeLength = 0;
			}

            List<string> variableInfoRegisterDeclNameList = new List<string>();
			{
                foreach (Info.VariableInfo variableInfo in classInfo.VariableInfoList)
                {
                    if (variableInfo.IsIgnoreReflection)
                    {
                        continue;
                    }
					variableInfoRegisterDeclNameList.Add($"variable_info_{variableInfo.Hash}");
                    GenerateVariableInfo(codeWriter, variableInfo, classInfo);
                }

                if (variableInfoRegisterDeclNameList.Count > 0)
                {
                    codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::VariableInfo> variable_info_table_{classInfo.Hash}[{variableInfoRegisterDeclNameList.Count}] = {{");
                    codeWriter.Push();
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
                    codeWriter.Pop();
                    codeWriter.WriteLine("};");
				}
            }

			List<string> functionInfoRegisterDeclNameList = new List<string>();
			{
                foreach (Info.FunctionInfo functionInfo in classInfo.FunctionInfoList)
                {
					if (functionInfo.IsIgnoreReflection == true)
                    {
                        continue;
                    }
					functionInfoRegisterDeclNameList.Add($"function_info_{functionInfo.Hash}");
                    GenerateFunctionInfo(codeWriter, functionInfo, classInfo);
                }

                if (functionInfoRegisterDeclNameList.Count > 0)
                {
                    codeWriter.WriteLine($"static constexpr const std::reference_wrapper<const nox::reflection::FunctionInfo> function_info_table_{classInfo.Hash}[{functionInfoRegisterDeclNameList.Count}] = {{");
                    codeWriter.Push();
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
                    codeWriter.Pop();
                    codeWriter.WriteLine("};");
				}
            }

            List<string> enumInfoRegisterDeclNameList = new List<string>();

			{
                foreach (Info.EnumInfo enumInfo in classInfo.EnumInfoList)
                {
                    if (enumInfo.IsReflection == false)
                    {
                        continue;
                    }
					enumInfoRegisterDeclNameList.Add($"enum_info_{enumInfo.Hash}");
                    GenerateEnumInfo(codeWriter, enumInfo, classInfo);
                }

                if (enumInfoRegisterDeclNameList.Count > 0)
                {
                    codeWriter.WriteLine($"enum_info_table_{classInfo.Hash}");
                }
            }

            codeWriter.WriteLine($"static constexpr const nox::reflection::ClassInfo class_info_{classInfo.Hash} = nox::reflection::ClassInfo(");
            codeWriter.Push();
            codeWriter.WriteLine($"nox::reflection::Typeof<{classInfo.FullName}>(),\t//\ttype");
            codeWriter.WriteLine($"U\"{classInfo.Name}\",\t//\tname");
            codeWriter.WriteLine($"U\"{classInfo.FullName}\",\t//\tfullName");
            codeWriter.WriteLine($"U\"{classInfo.Namespace}\",\t//\tnamespace");
            codeWriter.WriteLine($"nox::reflection::GetInvalidType(),\t//\texternal_class_type");
            if (classInfo.BaseTypeInfoList.Count > 0)
            {
                codeWriter.WriteLine($"base_type_table_{classInfo.Hash},\t//\tbase_type_list");
            }
            else
            {
                codeWriter.WriteLine("nullptr,\t//\tbase_type_list");
            }
            codeWriter.WriteLine($"{classInfo.BaseTypeInfoList.Count.ToString()},\t//\tbase_type_length");

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

            if (classInfo.ClassInfoList.Count > 0)
            {
                codeWriter.WriteLine($"internal_type_table_{classInfo.Hash},\t//\tinternal_type_list");
            }
            else
            {
                codeWriter.WriteLine("nullptr,\t//\tinternal_type_list");
            }
            codeWriter.WriteLine($"{classInfo.ClassInfoList.Count.ToString()}\t//\tinternal_type_length");

            codeWriter.Pop();
            codeWriter.WriteLine(");");

            codeWriter.Pop();
            codeWriter.WriteLine("};");
            codeWriter.WriteNewLine();
		}

#endregion

#endregion
	}
}
