using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public class GlobalInfo : BaseInfo
    {
        public string Namespace { get; set; } = string.Empty;

        public List<FieldInfo> FieldInfoList { get; set; } = new List<FieldInfo>();
        public List<MethodInfo> MethodInfoList { get; set; } = new List<MethodInfo>();
    }

    public interface IHolder
    {
        public List<EnumInfo> EnumInfoList { get; } 
        public List<ClassUnionInfo> TypeInfoList { get; } 
        public List<VariableInfo> VariableInfoList { get; } 
        public List<FunctionInfo> FunctionInfoList { get; }
    }

    public class DeclHolder : Info.IBaseInfo, IHolder
    {
        public required string Namespace { get; init; }

        public List<DeclHolder> DeclHolderList { get; } = new List<DeclHolder>();
        public List<EnumInfo> EnumInfoList { get; } = new List<EnumInfo>();
        public List<ClassUnionInfo> TypeInfoList { get; } = new List<ClassUnionInfo>();
        public List<VariableInfo> VariableInfoList { get; } = new List<VariableInfo>();
        public List<FunctionInfo> FunctionInfoList { get; } = new List<FunctionInfo>();

        public override string ToString() => Namespace;
    }

    public class GlobalInfoContainer
    {
        public GlobalInfo? Current
        {
            get
            {
                _InfoStack.TryPeek(out GlobalInfo? globalInfo);
                return globalInfo;
            }
        }
        public List<GlobalInfo> InfoList { get; private set; } = new List<GlobalInfo>();

        private Dictionary<string, GlobalInfo> _GlobalInfoDic = new Dictionary<string, GlobalInfo>();

        private Stack<GlobalInfo> _InfoStack = new Stack<GlobalInfo>();

        public void Push(string scope)
        {
            _InfoStack.Push(GetCreate(scope));
        }

        public void Pop()
        {
            _InfoStack.Pop();
        }

        private GlobalInfo GetCreate(string scope)
        {
            if(_GlobalInfoDic.TryGetValue(scope, out GlobalInfo? globalInfo) == true)
            {
                return globalInfo;
            }
            _GlobalInfoDic.Add(scope, globalInfo = new GlobalInfo() { Namespace = scope, Module = new Module() {  ModuleName = ""} });
            InfoList.Add(globalInfo);
            return globalInfo;
        }
    }
}
