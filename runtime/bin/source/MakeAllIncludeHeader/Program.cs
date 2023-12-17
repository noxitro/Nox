using System;
using System.Xml;

namespace MakeAllIncludeHeader
{
    static class Entry
    {
        private enum ArgsCategory : byte
        {
            ProjectDir,
            ProjectName,
            _Max
        }

        /// <summary>
		/// AllIncludeファイルを作成する
		/// </summary>
		/// <param name="projectFile"></param>
		/// <param name="outputDirectory"></param>
		/// <returns></returns>
		private static bool CreateAllIncludeSourceFile(string projectDir, string projectName)
        {
            string outputDirectory = projectDir;
            string projectFilePath = System.IO.Path.GetFullPath($"{projectDir}/{projectName}.vcxproj");

            XmlDocument doc = new XmlDocument();
            doc.Load(projectFilePath); // ここにあなたの.vcxprojファイルのパスを入力してください

            XmlNamespaceManager manager = new XmlNamespaceManager(doc.NameTable);
            manager.AddNamespace("ns", "http://schemas.microsoft.com/developer/msbuild/2003");

            XmlNodeList? elemList = doc.SelectNodes("//ns:ClInclude", manager);
            if (elemList == null)
            {
                Console.WriteLine("CIIncludeの項目を取得できませんでした");
                return false;
            }

            const string allIncludeFileName = "all_include";

            List<string> includeFileNameList = new List<string>();
            for (int i = 0; i < elemList.Count; i++)
            {
                var includeAttribute = elemList[i]?.Attributes?["Include"];
                if (includeAttribute == null)
                {
                    continue;
                }

                string includeFileName = includeAttribute.Value;
                //  stdafx.hと$projectname$.hは除外
                if (includeFileName == "stdafx.h" || 
                    includeFileName == $"{projectName}.h" ||
                    includeFileName == $"{allIncludeFileName}.h" 

                    )
                {
                    continue;
                }

                includeFileNameList.Add(includeFileName);
            }

            //	ヘッダファイル
            string headerFilePath = Path.GetFullPath(string.Format($"{outputDirectory}/{allIncludeFileName}.h"));
            using (CodeWriter codeWriter = new CodeWriter(headerFilePath))
            {
                foreach (string includeFileName in includeFileNameList)
                {
                    codeWriter.WriteLine(string.Format("#include\t\"{0}\"", includeFileName));
                }
            }

            //	ソースファイル
            string filePath = Path.GetFullPath(string.Format($"{outputDirectory}/{allIncludeFileName}.cpp"));

            using (CodeWriter codeWriter = new CodeWriter(filePath))
            {
                codeWriter.WriteLine("#include\t\"stdafx.h\"");
                codeWriter.WriteNewLine();
                codeWriter.WriteLine($"#include\t\"{allIncludeFileName}.h\"");
            }

            return true;
        }

        static void Main(string[] args)
        {
        //    args = new string[] { "D:\\github\\Nox\\runtime\\app\\", "app" };

            if (args.Length < (int)ArgsCategory._Max)
            {
                Console.WriteLine("引数が足りません");
                return;
            }

            CreateAllIncludeSourceFile(args[(int)ArgsCategory.ProjectDir], args[(int)ArgsCategory.ProjectName]);
            Console.WriteLine("MakeAllIncludeFile完了");
        }
    }
}