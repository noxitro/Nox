using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    /// <summary>
    /// 変数情報
    /// </summary>
    public class VariableInfo : BaseInfo
    {
        public override TypeInfoKind TypeInfoKind => TypeInfoKind.Variable;
        public required string Name { get; init; }
        public required string FullName { get; init; }

        /// <summary>
        /// オフセット
        /// </summary>
        public required long Offset { get; init; }

        /// <summary>
        /// ビットフィールド
        /// </summary>
        public required int BitWith { get; init; }

        public override string ToString() => FullName;

        public virtual bool IsTemplate { get; } = false;

        public required IReadOnlyList<AttributeInfo> AttributeInfoList { get; init; }
        public required AccessLevel AccessLevel { get; init; }
    }

    public class TemplateVariableInfo : VariableInfo
    {
        public required IReadOnlyList<string> SpecializationsList { get; init; }
        public override bool IsTemplate { get; } = true;
    }
}
