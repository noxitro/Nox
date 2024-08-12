using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    /// <summary>
    /// 関数情報
    /// </summary>
    public class FunctionInfo : Info.BaseInfo
    {
        public required string Name { get; init; }
        public required string FullName { get; init; }

        public required uint NumArguments { get; init; }

        public required uint NumDefaultArguments { get; init; }

        public override string ToString() => FullName;

        public required bool IsInline { get; init; }
        public required bool IsConstexpr { get; init; }
        public required bool IsVirtual { get; init; }
        public required bool IsPureVirtual { get; init; }
        public required bool IsConsteval { get; init; }

        public required IReadOnlyList<AttributeInfo> AttributeInfoList { get; init; }

        public virtual bool IsTemplated { get; } = false;
    }

    public class TemplateFunctionInfo : FunctionInfo
    {
        /// <summary>
        /// 特殊化名リスト
        /// </summary>
        public required IReadOnlyList<string> SpecializationsList { get; init; }

        public override bool IsTemplated { get; } = true;
    }
}
