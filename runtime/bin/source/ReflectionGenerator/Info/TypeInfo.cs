using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public struct TypeData
    {
        private readonly ClangSharp.Interop.CXType _RawValue;


		public required ClangSharp.Interop.CXType RawValue
        {
            readonly get => _RawValue;
            init
            {
                _RawValue = value;

                IsConst = _RawValue.IsConstQualified;
				RefQualifierKind = _RawValue.CXXRefQualifier;
			}
		}

        public bool IsConst { readonly get; init; }
		public ClangSharp.Interop.CXRefQualifierKind RefQualifierKind { readonly get; init; }
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
	/// 組み込み型情報
	/// </summary>
	public sealed class PrimitiveTypeInfo : TypeInfo
    {

    }

    /// <summary>
    /// クラス union情報
    /// </summary>
    public class ClassInfo : TypeInfo, IHolder
    {
        public List<EnumInfo> EnumInfoList { get; } = new List<EnumInfo>();
        public List<ClassInfo> ClassInfoList { get; } = new List<ClassInfo>();
        public List<VariableInfo> VariableInfoList { get; } = new List<VariableInfo>();
        public List<FunctionInfo> FunctionInfoList { get; } = new List<FunctionInfo>();

        public TypeInfo? ParentTypeInfo { get; set; } = null;

        public List<TypeInfo> BaseTypeInfoList { get; }  = new List<TypeInfo>();

        /// <summary>
        /// Privateメンバもリフレクション対象
        /// </summary>
        public bool IsPrivateReflection { get; set; } = false;

		/// <summary>
		/// Attributeクラスか
		/// NOTE:   nox::reflection::IAttributeを継承しているかどうか
		/// </summary>
		public required bool IsAttribute { get; init; } 

        /// <summary>
        /// nox::reflection::ReflectionObject継承クラスか
        /// </summary>
        public bool IsReflectionObject { get; set; }
	}

    public class TemplateClassUnionInfo : ClassInfo
    {
        //  
        public required IReadOnlyList<string> SpecializationsList { get; init; } 
    }
}
