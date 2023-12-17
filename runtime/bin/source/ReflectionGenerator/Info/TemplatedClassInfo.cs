using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public class TemplatedClassInfo : BaseInfo
    {
        public struct TemplateArgument
        {
            public required string FullName { get; init; }
        }

        public required string FullName { get; init; }

        /// <summary>
        /// テンプレートパラメータリストテーブル
        /// </summary>
   //     public required TemplateParam[][] TemplateParamListTable { get; init; }

        public required TemplateArgument[] TemplateArgumentList { get; init; }

        public required ClangSharp.Interop.CXCursor DefinitionCursor { get; init; }
    }
}
