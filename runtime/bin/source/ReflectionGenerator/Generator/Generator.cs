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
        private enum GEN_FILE_KIND : byte
        {
            CLASS_UNION,
            GLOBAL,
        }

        #region 非公開フィールド
        private string _BaseDirectory = string.Empty;
        #endregion

        #region 公開プロパティ
        public required Dictionary<string, List<Info.DeclHolder>> TypeInfoListWithModuleNameDict { get; init; }

        /// <summary>
        /// 1ファイルに定義する数
        /// </summary>
        public uint DevisionInfoCount { private get; init; } = 10;

        public required string OutputDirectory { private get; init; }
        public required string BuildSpec { private get; init; }
        public required string Platform { private get; init; }
        public required string BuildSpecDefine { private get; init; }
        public required string PlatformDefine { private get; init; }
        public required IReadOnlyList<string> TargetModuleNameList { get; init; }
        #endregion

        #region 公開メソッド
        public bool Generate()
        {
            //  出力先ディレクトリを決定
            _BaseDirectory = Path.GetFullPath($"{OutputDirectory}/{BuildSpec}/{Platform}/");

            if (Directory.Exists(_BaseDirectory) == false)
            {
                //  ディレクトリが存在しない場合作成
                Directory.CreateDirectory(_BaseDirectory);
                return false;
            }

            //  直列
#if true

            foreach (var pair in TypeInfoListWithModuleNameDict)
            {
                if (TargetModuleNameList.Contains(pair.Key) == false)
                {
                    continue;
                }

                Generate(pair.Key, pair.Value);
            }
#else
            System.Threading.Tasks.Parallel.ForEach(TypeInfoListWithModuleNameDict.Keys, key =>
            {
                if (TargetModuleNameList.Contains(key) == false)
                {
                    return;
                }

                Generate(key, TypeInfoListWithModuleNameDict[key]);
            }
            );
#endif

            return true;
        }
#endregion

        private struct DeclData
        {
            public DeclData() { }

            public string DeclBuffer { get; set; } = string.Empty;
            public List<string> memberList { get; } = new List<string>();
        }

        #region 非公開メソッド
        private void Generate(string moduleName, IReadOnlyList<Info.DeclHolder> infoList)
        {
            int maxThreadID = Util.MAX_THREAD_ID;
      //      maxThreadID = 1;
            List<DeclData>[] classBuffer = new List<DeclData>[maxThreadID];
            List<DeclData>[] globalDeclBuffer = new List<DeclData>[maxThreadID];

            for (int i = 0; i < maxThreadID; ++i)
            {
                classBuffer[i] = new List<DeclData>();
                globalDeclBuffer[i] = new List<DeclData>();
            }
            
            foreach (Info.DeclHolder info in infoList)
            {
                GenerateDeclInfo(info, ref globalDeclBuffer[System.Threading.Thread.CurrentThread.ManagedThreadId]);

                foreach(Info.ClassUnionInfo classUnionInfo in info.TypeInfoList)
                {
                    GenerateClassUnion(classUnionInfo, ref classBuffer[System.Threading.Thread.CurrentThread.ManagedThreadId]);
                }
            }

            //  ファイル出力
            
        }

        private void GenerateFile(string moduleName, GEN_FILE_KIND kind, List<string> buffer)
        {
            string path = "";
            using(CodeWriter codeWriter = new CodeWriter(path))
            {
                codeWriter.WriteLine($"#if {PlatformDefine}");
                codeWriter.WriteLine($"#if {BuildSpecDefine}");
                codeWriter.WriteNewLine();

                //  header
                codeWriter.WriteLine("#pragma once");
                codeWriter.WriteLine("//\tdo not edit");
                codeWriter.WriteLine("//\twritten from ReflectionGenerator");
                codeWriter.WriteNewLine();

                //  定義
                codeWriter.WriteLine("namespace");
                codeWriter.PushScope(CodeWriter.ScopeType.Decl);



                codeWriter.PopScope();


                //  登録処理


                //  登録解除処理

                codeWriter.WriteLine($"#endif\t//\t{BuildSpecDefine}");
                codeWriter.WriteLine($"#endif\t//\t{PlatformDefine}");

            }
        }

        private void GenerateDeclInfo(Info.DeclHolder info, ref List<DeclData> bufferList)
        {
            foreach(Info.FunctionInfo functionInfo in info.FunctionInfoList)
            {
                //  リフレクション対象か？
                if(functionInfo.IsReflection == false)
                {
                    continue;
                }


            }
        }

        private void GenerateClassUnion(Info.ClassUnionInfo info, ref List<DeclData> bufferList)
        {
            //  リフレクション対象か？
            if (info.IsReflection == false)
            {
                return;
            }
        }

        #endregion
    }
}
