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
            public required string ArtifactName { get; init; }

            public override int GetHashCode()
            {
                return ArtifactName.GetHashCode();
            }

            bool IEquatable<ARTIFACT_INFO>.Equals(ARTIFACT_INFO other)
            {
                return GetHashCode() == other.GetHashCode();
            }
        }

        #endregion
        const string ClassInfoStr = "ClassInfo";
        const string GlobalDeclStr = "GlobalDecl";
        const int NUM_DIGIT_INDEX = 2;

        private const string NOX_REFLECTION_GEN_NAMESPACE_STR = "nox::reflection::gen";

        private enum GEN_FILE_KIND : byte
        {
            CLASS_UNION,
            GLOBAL,
        }


        #region 非公開フィールド
        private string _BaseDirectory = string.Empty;

        #endregion

        #region 公開プロパティ
        /// <summary>
        /// Key: モジュール名 Value: モジュールごとの型情報リスト
        /// </summary>
        public required Dictionary<string, List<Info.NamespaceDeclInfo>> TypeInfoListWithArtifactNameDict { get; init; }

        /// <summary>
        /// 1ファイルに定義する数
        /// </summary>
        public uint DivisionInfoCount { private get; init; } = 10;

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
                    continue;
                }

                using (CodeWriter codeWriter = new CodeWriter(genHeaderFilePath))
                {
                    codeWriter.WriteLineCopyRight();
                    codeWriter.WriteLineHeader();

                    codeWriter.WriteLinePPIf(ConfigurationDefine);
                    codeWriter.WriteLinePPIf(PlatformDefine);

                    codeWriter.WriteNamespace(NOX_REFLECTION_GEN_NAMESPACE_STR);
                    codeWriter.PushScope(CodeWriter.ScopeType.Define);

                    for (int i = 0; i < DivisionInfoCount; ++i)
                    {
                        string indexStr = i.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                        codeWriter.WriteLine($"void\tRegister_{Platform}_{Configuration}_{artifactName}_{ClassInfoStr}_{indexStr}();");
                        codeWriter.WriteLine($"void\tUnregister_{Platform}_{Configuration}_{artifactName}_{ClassInfoStr}_{indexStr}();");

                        codeWriter.WriteLine($"void\tRegister_{Platform}_{Configuration}_{artifactName}_{GlobalDeclStr}_{indexStr}();");
                        codeWriter.WriteLine($"void\tUnregister_{Platform}_{Configuration}_{artifactName}_{GlobalDeclStr}_{indexStr}();");
                    }

                    codeWriter.PopScope();

                    codeWriter.WriteLinePPEndIf(PlatformDefine);
                    codeWriter.WriteLinePPEndIf(ConfigurationDefine);
                }
            }

            //  モジュールごとのソースファイルを生成
            //  ~/gen/BuildSpec/Platform/gen_BuildSpec_Platform_ModuleName_{index}.cpp
            foreach(var pair in TypeInfoListWithArtifactNameDict)
            {
				this.Generate(pair.Key, pair.Value);
            }

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
                                for (int divideIndex = 0; divideIndex < DivisionInfoCount; ++divideIndex)
                                {
                                    string indexStr = divideIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_{moduleInfo.ArtifactName}_{ClassInfoStr}_{indexStr}();");
                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_{moduleInfo.ArtifactName}_{GlobalDeclStr}_{indexStr}();");
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
                                for (int divideIndex = 0; divideIndex < DivisionInfoCount; ++divideIndex)
                                {
                                    string indexStr = divideIndex.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unegister_{Platform}_{Configuration}_{moduleInfo.ArtifactName}_{ClassInfoStr}_{indexStr}();");
                                    codeWriter.WriteLine($"::{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unegister_{Platform}_{Configuration}_{moduleInfo.ArtifactName}_{GlobalDeclStr}_{indexStr}();");
                                }
                                codeWriter.WriteLineEndRegion(moduleInfo.ArtifactName);
                            }
                            codeWriter.PopScope();
                        }
                    }

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

		private struct DeclData
        {
            public DeclData() { }

            /// <summary>
            /// 定義文字列
            /// </summary>
            public string DeclBuffer { get; set; } = string.Empty;

            public List<string> RegisterNameList { get; } = new List<string>();
            //    public List<string> memberList { get; } = new List<string>();
        }

        #region 非公開メソッド
        private void Generate(string artifactName, IReadOnlyList<Info.NamespaceDeclInfo> infoList)
        {
            int maxThreadID = Util.MAX_THREAD_ID;
            maxThreadID = 1;
            List<DeclData>[] classBufferListTable = new List<DeclData>[maxThreadID];
            List<DeclData>[] globalDeclBufferListTable = new List<DeclData>[maxThreadID];

            for (int i = 0; i < maxThreadID; ++i)
            {
                classBufferListTable[i] = new List<DeclData>();
                globalDeclBufferListTable[i] = new List<DeclData>();
            }
            
            void process(Info.NamespaceDeclInfo decl, int threadIndex)
            {
                {
                    GenerateDeclInfo(decl, ref globalDeclBufferListTable[threadIndex]);
                    foreach (Info.ClassInfo classUnionInfo in decl.ClassInfoList)
                    {
                    //    GenerateClassUnion(classUnionInfo, ref classBufferListTable[threadIndex]);
                    }
                }
            }


            //      System.Threading.Tasks.Parallel.For(0, infoList.Count, index => process(infoList[index], 0));

            infoList.ForEach(index => process(index, 0));

            //  バッファ統合
            List <DeclData> globalDeclBufferList = new List<DeclData>();
            foreach (List<DeclData> bufferList in globalDeclBufferListTable)
            {
                globalDeclBufferList.AddRange(bufferList);
            }

            List<DeclData> classBufferList = new List<DeclData>();
            foreach (List<DeclData> bufferList in classBufferListTable)
            {
                classBufferList.AddRange(bufferList);
            }

            //  ファイル出力
            const int NumMaxFile = 10;
            for(int i = 0; i < NumMaxFile ; ++i)
            {
				//MEMO 追加のインクルードディレクトリにプロジェクトディレクトリを指定している必要がある
				string baseHeaderFilePath = $"gen_{Platform}_{Configuration}_{artifactName}.h";

				{
                    string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/{artifactName}/{artifactName}_{ClassInfoStr}_{i}.cpp");
                    GenerateFile(path, baseHeaderFilePath, i, artifactName, ClassInfoStr, globalDeclBufferList);
                }

                {
					string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/{artifactName}/{artifactName}_{GlobalDeclStr}_{i}.cpp");
					GenerateFile(path, baseHeaderFilePath, i, artifactName, GlobalDeclStr, classBufferList);
                }

			}
        }

        private void GenerateFile(string path, string baseHeaderFilePath, int index, string artifactName, string genKindStr, IReadOnlyList<DeclData> declList)
        {
            string indexStr = index.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

            using CodeWriter codeWriter = new CodeWriter(path);

            codeWriter.WriteLineSource();
            codeWriter.WriteNewLine();
            codeWriter.WriteIncludeStdafx();
            codeWriter.WriteLine($"#include\t\"{baseHeaderFilePath}\"");
            codeWriter.WriteLine($"#include\t\"../../../support_functions.h\"");
			codeWriter.WriteNewLine();

            //  プリプロセッサ
            codeWriter.WriteLinePPIf(ConfigurationDefine);
            codeWriter.WriteLinePPIf(PlatformDefine);
            codeWriter.WriteNewLine();
            
            //  定義
            codeWriter.WriteLine($"namespace {NOX_REFLECTION_GEN_NAMESPACE_STR}");
            {
                codeWriter.PushScope(CodeWriter.ScopeType.Define);

                foreach (DeclData declData in declList)
                {
                    codeWriter.WriteLine(declData.DeclBuffer);
                    codeWriter.WriteNewLine();
                }

                codeWriter.PopScope();
            }

            //  登録処理
            codeWriter.WriteLine($"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register_{Platform}_{Configuration}_{artifactName}_{genKindStr}_{indexStr}()");
            codeWriter.PushScope(CodeWriter.ScopeType.Define);

            foreach (DeclData declData in declList)
            {
                foreach (string registerName in declData.RegisterNameList)
                {
                    codeWriter.WriteLine($"nox::reflection::Register({registerName});");
                }
            }

            codeWriter.PopScope();


            //  登録解除処理
            codeWriter.WriteLine($"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister_{Platform}_{Configuration}_{artifactName}_{genKindStr}_{indexStr}()");
            codeWriter.PushScope(CodeWriter.ScopeType.Define);
            foreach (DeclData declData in declList)
            {
                foreach (string registerName in declData.RegisterNameList)
                {
                    codeWriter.WriteLine($"nox::reflection::Unregister({registerName});");
                }
            }

            codeWriter.PopScope();

            codeWriter.WriteLinePPEndIf(PlatformDefine);
            codeWriter.WriteLinePPEndIf(ConfigurationDefine);
        }

        private void GenerateDeclInfo(Info.NamespaceDeclInfo info, ref List<DeclData> bufferList)
        {
            foreach (Info.VariableInfo functionInfo in info.VariableInfoList)
            {
                if(functionInfo.IsReflection == false)
                {
                    continue;
                }

                string buffer = GenerateVariableInfo(functionInfo, null);
                bufferList.Add(new DeclData() { DeclBuffer = buffer, RegisterNameList = { GetVariableInfoDeclName(functionInfo) } });
            }

            foreach (Info.FunctionInfo functionInfo in info.FunctionInfoList)
            {
                //  リフレクション対象か？
                if(functionInfo.IsReflection == false)
                {
                    continue;
                }

            }
        }

        private void GenerateClassUnion(string name, Info.ClassInfo info, ref List<DeclData> bufferList)
        {


            //  リフレクション対象か？
            if (info.IsReflection == false)
            {
                return;
            }

            for (int i = 0; i < info.VariableInfoList.Count; ++i)
            {
                //    GenerateVariableInfo(info.VariableInfoList[i], i);
            }

            {
                for (int i = 0; i < info.EnumInfoList.Count; ++i)
                {

                }
            }
        }

        private static string GetAttributeTableDeclName(string hash) => $"attr_table_{hash}";
        private static string GetVariableInfoDeclName(Info.VariableInfo variableInfo) => $"variable_info_{variableInfo.Hash}";

        /// <summary>
        /// 属性テーブル
        /// </summary>
        /// <param name="attributeInfoList"></param>
        /// <param name="hash"></param>
        /// <returns></returns>
        private string GenerateAttributeInfo(IReadOnlyList<Info.AttributeInfo> attributeInfoList, string hash)
        {
            System.Diagnostics.Debug.Assert(attributeInfoList.Count > 0, "属性情報が存在しません。");

            string buffer = string.Empty;

            int index = 0;
            foreach(Info.AttributeInfo attributeInfo in attributeInfoList)
            {
                string attr_decl_name = getAttrInfoDeclName(hash, index);

                switch (attributeInfo)
                {
                    case Info.EngineAnnotateAttribute engineAnnotateAttribute:

                        string fullDecl = engineAnnotateAttribute.Value;
                        buffer += $"static constexpr nox::reflection::ReflectionObject {attr_decl_name} = {fullDecl};";
                        break;

                    default:
                        buffer += $"static constexpr nox::reflection::attr::StandardAttribute {attr_decl_name} = nox::reflection::attr::StandardAttribute(0);\n";
                        break;

                }

                ++index;
            }

            buffer += $"static constexpr const std::reference_wrapper<const nox::reflection::ReflectionObject> {GetAttributeTableDeclName(hash)}[{attributeInfoList.Count.ToString()}] = {{";
            for(int i = 0; i < attributeInfoList.Count; ++i)
            {
                if(i == attributeInfoList.Count - 1)
                {
                    buffer += $"{getAttrInfoDeclName(hash, i)}\n";
                }
                else
                {
                    buffer += $"{getAttrInfoDeclName(hash, i)},\n";
                }
            }
            buffer += "};";

            return buffer;

            static string getAttrInfoDeclName(string hash, int index)
            {
                return $"attr_decl_{hash}_{index}";
            }
        }

        #region 定義生成群
        /// <summary>
        /// 変数情報を生成
        /// </summary>
        /// <param name="variableInfo"></param>
        /// <returns></returns>
        private string GenerateVariableInfo(Info.VariableInfo variableInfo, Info.ClassInfo? parentUserDefinedCompoundTypeInfo)
        {
            string buffer = string.Empty;

            //  属性定義

            //  属性が存在するか
            bool enabledAttribute = variableInfo.AttributeInfoList.Count > 0;

            string attrBuffer;
			if (enabledAttribute == true)
            {
                attrBuffer = GenerateAttributeInfo(variableInfo.AttributeInfoList, variableInfo.Hash);
                buffer += attrBuffer;
                buffer += "\n";
            }
            else
            {
                attrBuffer = "nullptr";
			}

            string attr_decl_name = $"attr_decl_{variableInfo.Hash}";

            buffer += "\n\n";

            //  メンバ変数
            if(variableInfo.IsStatic == false)
            {
                if(variableInfo.TypeData.RawValue.CXXRefQualifier == ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_None)
				{
					buffer += $"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoMember<&{variableInfo.FullName}>";
				}
				else
				{
					buffer += $"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoMemberRef";
				}
			}
			//  グローバル変数
			else
            {
				if (variableInfo.TypeData.RawValue.CXXRefQualifier == ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_None)
				{
					buffer += $"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoGlobal<&{variableInfo.FullName}>";
				}
				else
				{
					buffer += $"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfoGlobalRef";
				}
			}
            
            buffer += "(\n";

            //  非参照型の場合
            if (variableInfo.TypeData.RawValue.CXXRefQualifier == ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_None)
            {
				//  オブジェクトポインタ
			//	buffer += $"\t&{variableInfo.FullName},\t//\t object_pointer\n";
            }
            else
            {
				buffer += $"\tnox::reflection::Typeof<decltype({variableInfo.FullName})>(),\t//\ttype\n";
				if (variableInfo.IsStatic == false)
				{
					System.Diagnostics.Debug.Assert(parentUserDefinedCompoundTypeInfo != null);
					buffer += $"\tnox::reflection::Typeof<decltype({parentUserDefinedCompoundTypeInfo.FullName})>(),\t//\towner type\n";
				}
			}
                
            //  各種名前
            buffer += $"\tU\"{variableInfo.Name}\",\t//\tname\n";
            buffer += $"\tU\"{variableInfo.FullName}\",\t//\tfullname\n";
            buffer += $"\tU\"{variableInfo.Namespace}\",\t//\tnamespace\n";

            buffer += $"\t{variableInfo.AccessLevel.GetRuntimeFqn()},\t//\taccess level\n";
            buffer += $"\t{variableInfo.BitWith.ToString()},\t//\tbit with\n";
            buffer += $"\t{variableInfo.Offset.ToString()},\t//\toffset\n";

            //  属性
            if (enabledAttribute == true)
            {
                buffer += "\tnullptr,\t//\tattribute\n";
            }
            else
            {
				buffer += $"\t{attrBuffer},\t//\tattribute\n";
            }


            buffer += $"\t{variableInfo.AttributeInfoList.Count.ToString()},\t//\tattribute length\n";

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
                        buffer += $"\t{variableAttributeFlagsStrList[0]},\t//\tvariable attribute flags\n";
                    }
                    else
                    {
                        buffer += "\tnox::util::BitOr(\n";
                        int lastIndex = variableAttributeFlagsStrList.Count - 1;

                        for (int i = 0; i < variableAttributeFlagsStrList.Count; ++i)
                        {
                            buffer += variableAttributeFlagsStrList[i];
                            if (i != lastIndex)
                            {
                                buffer += ",\n";
                            }
                            else
                            {
                                buffer += "\n";
                            }
                        }
                        buffer += "),\n";
                    }
                }
                else
                {
                    buffer += "nox::reflection::VariableAttributeFlag::None,\t//\tvariable attribute flags\n";
                }
            }


            string variableTypeStr = $"decltype({variableInfo.FullName})";
            string variableRemoveConstTypeStr = $"std::remove_const_t<decltype({variableInfo.FullName})>";

            //  getter, setterの記述
            //  reflection_generatedプロジェクトのsupport_functionsにあるマクロを使用する
            if (variableInfo.IsStatic == false)
            {
                //  メンバ

                //string instanceTypeStr = $"\tnox::MemberObjectPointerClassType<decltype({variableInfo.FullName})>";

                // setter
                buffer += $"\tNOX_VARIABLE_INFO_SETTER_MEMBER({variableInfo.FullName}),\t//\tsetter\n";
                // getter
                buffer += $"\tNOX_VARIABLE_INFO_GETTER_MEMBER({variableInfo.FullName}),\t//\tgetter\n";
                // getter address
                buffer += $"\tNOX_VARIABLE_INFO_GETTER_ADDRESS_MEMBER({variableInfo.FullName}),\t//\tgetter address\n";

                // setter subscripts
                buffer += $"\tNOX_VARIABLE_INFO_SETTER_SUBSCRIPT_MEMBER({variableInfo.FullName}),\t//\tsetter subscript\n";

                // getter subscripts
                buffer += $"\tNOX_VARIABLE_INFO_GETTER_SUBSCRIPT_MEMBER({variableInfo.FullName}),\t//\tgetter subscript\n";
                // getter address subscripts
                buffer += $"\tNOX_VARIABLE_INFO_GETTER_ADDRESS_SUBSCRIPT_MEMBER({variableInfo.FullName})\t//\tgetter address subscript\n";
            }
            else
            {
				//  グローバル

				// setter
				buffer += $"\tNOX_VARIABLE_INFO_SETTER_GLOBAL({variableInfo.FullName}),\t//\tsetter\n";
				// getter
				buffer += $"\tNOX_VARIABLE_INFO_GETTER_GLOBAL({variableInfo.FullName}),\t//\tgetter\n";
				// getter address
				buffer += $"\tNOX_VARIABLE_INFO_GETTER_ADDRESS_GLOBAL({variableInfo.FullName}),\t//\tgetter address\n";

				// setter subscripts
				buffer += $"\tNOX_VARIABLE_INFO_SETTER_SUBSCRIPT_GLOBAL({variableInfo.FullName}),\t//\tsetter subscript\n";

				// getter subscripts
				buffer += $"\tNOX_VARIABLE_INFO_GETTER_SUBSCRIPT_GLOBAL({variableInfo.FullName}),\t//\tgetter subscript\n";
				// getter address subscripts
				buffer += $"\tNOX_VARIABLE_INFO_GETTER_ADDRESS_SUBSCRIPT_GLOBAL({variableInfo.FullName})\t//\tgetter address subscript\n";
			}
#if false
            //  非メンバ
            if (variableInfo.IsStatic == true)
            {
				//  setter
				if (variableInfo.IsConstexpr == true || variableInfo.TypeData.RawValue.IsConstQualified == true)
                {
                    buffer += $"\tnullptr,\t//\tsetter\n";
                }
                else
                {
                    buffer += "\t+[](nox::not_null<const void*> value){";
                    buffer += $"{variableInfo.Name} = *static_cast<{variableTypeStr}*>(value.get());";
                    buffer += "},\t//\tsetter\n";
                }

                //  getter

                //  参照型の場合
                buffer += "\t+[](nox::not_null<void*> out){";
                buffer += $"*static_cast<{variableRemoveConstTypeStr}*>(out.get()) = {variableInfo.FullName};";
                buffer += "},\t//\tgetter\n";
               

                //  getter address
                //  配列の場合は
                if (variableInfo.TypeData.RawValue.IsArray() == true)
                {
                    buffer += "\tnullptr,\t//\tgetter address\n";
                }
                else
                {
                    buffer += "\t+[](nox::not_null<void*> out){";
                    if (variableInfo.TypeData.RawValue.IsConstQualified == true)
                    {
                        buffer += $"*static_cast<std::decay_t<decltype(&{variableInfo.FullName})>*>(out.get()) = &{variableInfo.FullName};";
                    }
                    else
                    {
                        buffer += $"*static_cast<std::decay_t<decltype(&{variableInfo.FullName})>*>(out.get()) = &{variableInfo.FullName};";
                    }
                    buffer += "},\t//\tgetter address\n";
                }

                //  setter subscripts
                buffer += "\tnullptr,\t//\tsetter subscripts\n";

                //  getter subscripts
                buffer += "\tnullptr\t//\tgetter subscripts\n";
            }
            //  メンバ
            else
            {
                
                string instanceType = $"\tnox::MemberObjectPointerClassType<decltype({variableInfo.FullName})>";

                //  setter
                if (variableInfo.TypeData.RawValue.IsConstQualified == true)
                {
                    buffer += $"\tnullptr,\t//\tsetter\n";
                }
                else
                {
                    buffer += "\t+[](nox::not_null<void*> instance, nox::not_null<const void*> value){";
                    buffer += $"static_cast<{instanceType}*>(const_cast<void*>(instance.get()))->{variableInfo.Name} = *static_cast<{variableTypeStr}*>(value.get());";
                    buffer += "},\t//\tsetter\n";
                }

                //  getter
                buffer += "\t+[](nox::not_null<void*> out, nox::not_null<const void*> instance){";
                buffer += $"*static_cast<{variableRemoveConstTypeStr}*>(out.get()) = static_cast<const {instanceType}*>(instance.get())->{variableInfo.Name}";
                buffer += "},\t//\tgetter\n";

                //  getter address
                buffer += "+[](nox::not_null<void*> out, nox::not_null<void*> instance){";
                if (variableInfo.TypeData.RawValue.IsConstQualified == true)
                {
                    buffer += $"*static_cast<std::decay_t<decltype(&{variableInfo.FullName})>*>(out.get()) = &static_cast<const {instanceType}*>(instance.get())->{variableInfo.FullName}";
                }
                else
                {
                    buffer += $"*static_cast<std::decay_t<decltype(&{variableInfo.FullName})>*>(out.get()) = &static_cast<{instanceType}*>(instance.get())->{variableInfo.FullName}";
                }
                buffer += "},\t//\tgetter address\n";

                //  setter subscripts
                buffer += "nullptr,\t//\tsetter subscripts\n";

                //  getter subscripts
                buffer += "nullptr\t//\tgetter subscripts\n";

            }
#endif
			buffer += ");";

            return buffer;
        }

        private string GenerateEnumeratorInfo( IReadOnlyList<Info.EnumInfo.EnumVariable> enumeratorInfoList)
        {
			string buffer = string.Empty;

            for(int i = 0; i < enumeratorInfoList.Count; i++)
            {
                buffer += "";
			}

			return buffer;
		}

        private string GenerateEnumInfo(string name, Info.EnumInfo enumInfo)
        {
            string buffer = string.Empty;

			return buffer;
        }
#endregion

#endregion
	}
}
