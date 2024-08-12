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
        public required string Name { get; init; }
        public required string FullName { get; init; }

        public override string ToString() => FullName;

        public virtual bool IsTemplate { get; } = false;
    }

    public class TemplateVariableInfo : VariableInfo
    {
        public required IReadOnlyList<string> SpecializationsList { get; init; }
        public virtual bool IsTemplate { get; } = true;
    }
}
