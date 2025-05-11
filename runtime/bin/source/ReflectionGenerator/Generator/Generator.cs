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
        const string UserDefinedCompoundTypeInfoStr = "UserDefinedCompoundTypeInfo";
        const string GlobalStr = "Global";
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
        public required Dictionary<string, List<Info.DeclHolder>> TypeInfoListWithArtifactNameDict { get; init; }

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

        public required IReadOnlyList<ARTIFACT_INFO> ModuleInfoList { get; init; }
        #endregion

        #region 非公開プロパティ
        #endregion

        #region 公開メソッド
        public bool Generate()
        {
            string baseGenHeaderFilePath = $"{OutputDirectory}/gen.h";

            //  出力先ディレクトリを決定
            _BaseDirectory = Path.GetFullPath($"{OutputDirectory}/gen/{Configuration}/{Platform}/");

            if (Directory.Exists(_BaseDirectory) == false)
            {
                //  ディレクトリが存在しない場合作成
                Directory.CreateDirectory(_BaseDirectory);
            }

            //  モジュールごとのディレクトリを作成
            //  ~/gen/BuildSpec/Platform/ModuleName
            string[] moduleDirectoryList = new string[ModuleInfoList.Count];

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
            for (int moduleListIndex = 0; moduleListIndex < ModuleInfoList.Count; ++moduleListIndex)
            {
                ARTIFACT_INFO moduleInfo = ModuleInfoList[moduleListIndex];
                string artifactName = moduleInfo.ArtifactName;

                string genHeaderFilePath = System.IO.Path.GetFullPath($"{moduleDirectoryList[moduleListIndex]}/gen_{Configuration}_{Platform}_{artifactName}.h");

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

                    codeWriter.WriteNamespace("nox::reflection::gen");
                    codeWriter.PushScope(CodeWriter.ScopeType.Define);

                    for (int i = 0; i < DivisionInfoCount; ++i)
                    {
                        string indexStr = i.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

                        codeWriter.WriteLine($"void\tRegister_{Configuration}_{Platform}_{artifactName}_{UserDefinedCompoundTypeInfoStr}_{indexStr}();");
                        codeWriter.WriteLine($"void\tUnregister_{Configuration}_{Platform}_{artifactName}_{UserDefinedCompoundTypeInfoStr}_{indexStr}();");

                        codeWriter.WriteLine($"void\tRegister_{Configuration}_{Platform}_{artifactName}_{GlobalStr}_{indexStr}();");
                        codeWriter.WriteLine($"void\tUnregister_{Configuration}_{Platform}_{artifactName}_{GlobalStr}_{indexStr}();");
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
                string unitySourceFilePath = System.IO.Path.GetFullPath($"{_BaseDirectory}/gen_{Configuration}_{Platform}.cpp");
                using (CodeWriter codeWriter = new CodeWriter(unitySourceFilePath))
                {
                    codeWriter.WriteLineCopyRight();
                    codeWriter.WriteLineSource();
                    codeWriter.WriteNewLine();

                    codeWriter.WriteIncludeStdafx();
                    codeWriter.WriteLineInclude($"{OutputProjectDirectory}/gen.h");

                    codeWriter.WriteNewLine();


                    codeWriter.WriteLinePPIf(ConfigurationDefine);
                    codeWriter.WriteLinePPIf(PlatformDefine);

                    codeWriter.WriteNamespace("nox::reflection::gen");
                    codeWriter.PushScope(CodeWriter.ScopeType.Define);

                    codeWriter.WriteLine($"void Register{Configuration}{Platform}");
                    codeWriter.PushScope(CodeWriter.ScopeType.Define);
                    for(int i = 0; i < ModuleInfoList.Count; ++i)
                    {
                        ARTIFACT_INFO moduleInfo = ModuleInfoList[i];
                        codeWriter.WriteLine($"//\t{moduleInfo.ArtifactName}");
                        codeWriter.WriteLine($"");
                    }
                    codeWriter.PopScope();


                    codeWriter.PopScope();

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
        private void Generate2(string artifactName, IReadOnlyList<Info.DeclHolder> infoList)
        {
        }

        private void Generate(string artifactName, IReadOnlyList<Info.DeclHolder> infoList)
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
            
            void process(Info.DeclHolder decl, int threadIndex)
            {
                {
                    GenerateDeclInfo(decl, ref globalDeclBufferListTable[threadIndex]);

                    foreach (Info.UserDefinedCompoundTypeInfo classUnionInfo in decl.TypeInfoList)
                    {
                        GenerateClassUnion(classUnionInfo, ref classBufferListTable[threadIndex]);
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
                string path = System.IO.Path.GetFullPath($"{_BaseDirectory}/{artifactName}_{i}.cpp");
                GenerateFile(path, i, GlobalStr, globalDeclBufferList);
                GenerateFile(path, i, UserDefinedCompoundTypeInfoStr, classBufferList);
            }
        }

        private void GenerateFile(string path, int index, string genKindStr, IReadOnlyList<DeclData> declList)
        {
            string baseHeaderFilePath = $"{OutputDirectory}/{Configuration}/{Platform}.h";

            string indexStr = index.ToString().PadLeft(NUM_DIGIT_INDEX, '0');

            using (CodeWriter codeWriter = new CodeWriter(path))
            {
                //  copy right
                codeWriter.WriteLineCopyRight();
                codeWriter.WriteNewLine();

                //  プリプロセッサ
                codeWriter.WriteLinePPIf(ConfigurationDefine);
                codeWriter.WriteLinePPIf(PlatformDefine);
                codeWriter.WriteNewLine();

                //  header
                codeWriter.WriteLineHeader();
                codeWriter.WriteNewLine();

                //  プリプロセッサ

                //  include
                codeWriter.WriteLine($"#include\t{baseHeaderFilePath}");
                codeWriter.WriteNewLine();

                //  定義
                codeWriter.WriteLine("namespace nox::reflection::gen");
                codeWriter.PushScope(CodeWriter.ScopeType.Define);

                foreach (DeclData declData in declList)
                {
                    codeWriter.WriteLine(declData.DeclBuffer);
                    codeWriter.WriteNewLine();
                }

                codeWriter.PopScope();

                //  登録処理
                codeWriter.WriteLine($"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Register{Configuration}{Platform}_{genKindStr}{indexStr}()");
                codeWriter.PushScope(CodeWriter.ScopeType.Define);

                codeWriter.WriteLine("nox::reflection::Reflection& manager = nox::reflection::Reflection::Instance();");
                foreach (DeclData declData in declList)
                {
                    foreach(string registerName in declData.RegisterNameList)
                    {
                        codeWriter.WriteLine($"manager.Register({registerName});");
                    }
                }

                codeWriter.PopScope();


                //  登録解除処理
                codeWriter.WriteLine($"void\t{NOX_REFLECTION_GEN_NAMESPACE_STR}::Unregister{Configuration}{Platform}_{genKindStr}{indexStr}()");
                codeWriter.PushScope(CodeWriter.ScopeType.Define);
                codeWriter.WriteLine("nox::reflection::Reflection& manager = nox::reflection::Reflection::Instance();");
                foreach (DeclData declData in declList)
                {
                    foreach (string registerName in declData.RegisterNameList)
                    {
                        codeWriter.WriteLine($"manager.Unregister({registerName});");
                    }
                }

                codeWriter.PopScope();


                codeWriter.WriteLinePPEndIf(PlatformDefine);
                codeWriter.WriteLinePPEndIf(ConfigurationDefine);
            }
        }

        private void GenerateDeclInfo(Info.DeclHolder info, ref List<DeclData> bufferList)
        {
            foreach (Info.VariableInfo functionInfo in info.VariableInfoList)
            {
                if(functionInfo.IsReflection == false)
                {
                    continue;
                }

                string buffer = GenerateVariableInfo(functionInfo);
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

        private void GenerateClassUnion(Info.UserDefinedCompoundTypeInfo info, ref List<DeclData> bufferList)
        {
            //  リフレクション対象か？
            if (info.IsReflection == false)
            {
                return;
            }

            for(int i = 0; i < info.VariableInfoList.Count; ++i)
            {
            //    GenerateVariableInfo(info.VariableInfoList[i], i);
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

            buffer += $"static constexpr const std::reference_wrapper<const nox::reflection::ReflectionObject> {GetAttributeTableDeclName(hash)}[{totalCount}] = {{";
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

        /// <summary>
        /// 変数情報を生成
        /// </summary>
        /// <param name="variableInfo"></param>
        /// <returns></returns>
        private string GenerateVariableInfo(Info.VariableInfo variableInfo)
        {
            string buffer = string.Empty;

            //  属性定義

            //  属性が存在するか
            bool enabledAttribute = variableInfo.AttributeInfoList.Count > 0;

            if(enabledAttribute == true)
            {
                string attrBuffer = GenerateAttributeInfo(variableInfo.AttributeInfoList, variableInfo.Hash);
                buffer += attrBuffer;
                buffer += "\n";
            }

            string attr_decl_name = $"attr_decl_{variableInfo.Hash}";

            buffer += "\n\n";

            buffer += $"static constexpr auto {GetVariableInfoDeclName(variableInfo)} = nox::reflection::detail::CreateVariableInfo<decltype(&{variableInfo.FullName})>";
            buffer += "(\n";

            buffer += $"&{variableInfo.FullName},\n";

            buffer += $"{variableInfo.Name},\n";
            buffer += $"{variableInfo.FullName},\n";
            buffer += $"{variableInfo.Namespace},\n";

            buffer += $"{variableInfo.AccessLevel.GetRuntimeFqn()},\n";
            buffer += $"{variableInfo.BitWith},\n";
            buffer += $"{variableInfo.Offset},\n";

            if (enabledAttribute == true)
            {
                buffer += "nullptr,\n";
            }
            else
            {
                buffer += $"{variableInfo.AttributeInfoList},\n";
            }

            buffer += $"{variableInfo.AttributeInfoList.Count.ToString()},\n";

            buffer += $"{variableInfo.IsConstexpr.ToString()},\n";
            buffer += $"false,\n";


            string variableTypeStr = $"decltype({variableInfo.FullName})";
            string variableRemoveConstType = $"std::remove_const_t<decltype({variableInfo.FullName})>";

            //  非メンバ
            if (variableInfo.IsStatic == true)
            {

            }
            //  メンバ
            else
            {
                
                string instanceType = $"nox::nox::FieldClassType<decltype({variableInfo.FullName})>";

                //  setter
                if (variableInfo.TypeData.RawValue.IsConstQualified == true)
                {
                    buffer += $"nullptr,\n";
                }
                else
                {
                    buffer += "+[](nox::not_null<void*> instance, nox::not_null<void*> value){";
                    buffer += $"static_cast<{instanceType}*>(instance.get())->{variableInfo.Name} = *static_cast<{variableTypeStr}*>(value.get());";
                    buffer += "},\n";
                }

                //  getter
                buffer += "+[](nox::not_null<void*> out, nox::not_null<void*> instance){";
                buffer += $"*static_cast<variableRemoveConstType*>(out.get()) = static_cast<const {instanceType}*>(instance.get())->{variableInfo.Name}";
                buffer += "},\n";

                //  getter address
                buffer += "+[](nox::not_null<void*> out, nox::not_null<void*> instance){";
                if (variableInfo.TypeData.RawValue.IsConstQualified == true)
                {
                    buffer += $"*static_cast<{instanceType}**>(out.get()) = &static_cast<const {instanceType}*>(instance.get())->{variableInfo.Name}";
                }
                else
                {
                    buffer += $"*static_cast<{instanceType}**>(out.get()) = &static_cast<{instanceType}*>(instance.get())->{variableInfo.Name}";
                }
                buffer += "},\n";

                //  setter array

            }

            buffer += ");";

            return buffer;
        }
        #endregion
    }
}
