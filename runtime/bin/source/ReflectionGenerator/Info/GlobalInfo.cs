using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{

    public interface IHolder
    {
        public List<EnumInfo> EnumInfoList { get; } 
        public List<UserDefinedCompoundTypeInfo> TypeInfoList { get; } 
        public List<VariableInfo> VariableInfoList { get; } 
        public List<FunctionInfo> FunctionInfoList { get; }
    }

    public class DeclHolder : Info.BaseInfo, IHolder
    {
        public override TypeInfoKind TypeInfoKind => TypeInfoKind.GlobalDecl;
        public required string Namespace { get; init; }

        public List<DeclHolder> DeclHolderList { get; } = new List<DeclHolder>();
        public List<EnumInfo> EnumInfoList { get; } = new List<EnumInfo>();
        public List<UserDefinedCompoundTypeInfo> TypeInfoList { get; } = new List<UserDefinedCompoundTypeInfo>();
        public List<VariableInfo> VariableInfoList { get; } = new List<VariableInfo>();
        public List<FunctionInfo> FunctionInfoList { get; } = new List<FunctionInfo>();

        public override string ToString() => Namespace;
    }

}
