using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public enum TemplateDeclKind
    {
        Type,
        Variable
    }

    public struct TemplateDeclInfo
    {
        public required TemplateDeclKind Kind { get; init; }
    }
}
