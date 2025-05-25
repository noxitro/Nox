using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public class Holder
    {
        public List<Info.VariableInfo> VariableInfoList { get; } = new List<Info.VariableInfo>();
    }

    public struct TypeData
    {
        public required ClangSharp.Interop.CXType RawValue { get; init; }
    }

    /// <summary>
    /// 型情報
    /// </summary>
    public class TypeInfo : BaseInfo
    {
        /// <summary>
        /// 修飾子付与情報
        /// </summary>
        public struct TypeQualifiersName
        {
            public required string Name { get; init; }
            public HashSet<string> TypeAliasNameHashSet { get; } = new HashSet<string>();

            public TypeQualifiersName()
            {

            }
        }

        #region 公開プロパティ
        public override TypeInfoKind TypeInfoKind => TypeInfoKind.Type;

        /// <summary>
        /// 完全型名
        /// </summary>
        public required string FullName { get; init; }

        public string Name { get; init; } = string.Empty;

        public required string Namespace { get; init; }


        /// <summary>
        /// 別名リスト
        /// </summary>
        public HashSet<string> TypeAliasNameHashSet { get; } = new HashSet<string>();

        /// <summary>
        /// 修飾子が付いた型名をKeyにした、別名ハッシュリスト
        /// </summary>
        public Dictionary<string, HashSet<string>> TypeQualifiersInfoHashSet { get; } = new Dictionary<string, HashSet<string>>();

        public bool IsTemplate { get; set; } = false;

        public virtual bool IsTemplateArgumentType { get; } = false;

        public required ClangSharp.Interop.CXType CXType
        {
            init { }
        }

        public required AccessLevel AccessLevel { get; init; }
        public override string ToString() => FullName;
        #endregion
    }

    /// <summary>
    /// クラス union情報
    /// </summary>
    public class UserDefinedCompoundTypeInfo : TypeInfo, IHolder
    {
        public List<EnumInfo> EnumInfoList { get; } = new List<EnumInfo>();
        public List<UserDefinedCompoundTypeInfo> TypeInfoList { get; } = new List<UserDefinedCompoundTypeInfo>();
        public List<VariableInfo> VariableInfoList { get; } = new List<VariableInfo>();
        public List<FunctionInfo> FunctionInfoList { get; } = new List<FunctionInfo>();

        public TypeInfo? ParentTypeInfo { get; set; } = null;

        /// <summary>
        /// Privateメンバもリフレクション対象
        /// </summary>
        public bool IsPrivateReflection { get; set; } = false;
    }

    public class TemplateClassUnionInfo : UserDefinedCompoundTypeInfo
    {
        //  
        public required IReadOnlyList<string> SpecializationsList { get; init; } 
    }
}
