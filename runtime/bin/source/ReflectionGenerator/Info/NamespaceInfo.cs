using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public class NamespaceInfo
    {
        public string Name { get; set; } = string.Empty;
        public bool IsInline { get; set; } = false;
    }

    public class NamespaceInfoStack : IStacker<NamespaceInfo>
    {

    }
}
