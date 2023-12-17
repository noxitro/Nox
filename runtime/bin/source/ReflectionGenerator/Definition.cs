using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    /// <summary>
    /// リフレクション対象
    /// </summary>
    public enum ReflectionTarget : ulong
    {
        /// <summary>
        /// リフレクションなし
        /// </summary>
        None = 0,

        /// <summary>
        /// リフレクション定義を付けたもの
        /// </summary>
        ReflectionDeclare = 1 << 0,

        /// <summary>
        /// リフレクション対象
        /// </summary>
        ReflectionObject = 1 << 1,

        /// <summary>
        /// リフレクション属性
        /// </summary>
        ReflectionAttribute = 1 << 2,

        /// <summary>
        /// 全てを対象
        /// </summary>
        All = 1 << 3
    }

    /// <summary>
    /// 型情報
    /// </summary>
    public class RuntimeType
    {
        public bool IsVoid => TypeKind == ClangSharp.Interop.CXTypeKind.CXType_Void;
        public string Name { get; init; } = string.Empty;
        public string FullName { get; init; } = string.Empty;
        public string Namespace { get; init; } = string.Empty;

        public string TemplateValueStr { get; set; } = string.Empty;

        public List<RuntimeType> RuntimeValueList { get; } = new List<RuntimeType>();

        private RuntimeAttribute _AttributeFlags = RuntimeAttribute.Invalid;

        public required ClangSharp.Interop.CXTypeKind TypeKind { private get; init; }

        public bool IsArray => TypeKind == ClangSharp.Interop.CXTypeKind.CXType_ConstantArray;

        public bool IsTemplate => TemplateDepth > 0 || TemplateIndex > 0;

        public int TemplateDepth { get; init; } = -1;
        public int TemplateIndex { get; init; } = -1;

        public required IReadOnlyList<RuntimeType> TemplateArgumentList { get; init; }

        public static readonly IReadOnlyList<RuntimeType> DummyList = new List<RuntimeType>();
        public static readonly RuntimeType Null = new RuntimeType() 
        {
           TemplateArgumentList = DummyList,
           TypeKind = ClangSharp.Interop.CXTypeKind.CXType_Invalid,
        };
    }

    /// <summary>
    /// Runtimeの定義型
    /// </summary>
    public enum RuntimeTypeKind : byte
    {
        Invalid,
        Void,
        S8,
        S16,
        S32,
        S64,
        U8,
        U16,
        U32,
        U64,
        C8,
        C16,
        C32,
        W16,
        Bool,

		Unexposed,
    }

    /// <summary>
    /// 型属性
    /// </summary>
    public enum RuntimeAttribute : byte
    {
        Invalid,
        Array,      //  添え字アクセスが可能
    }

    public enum AccessLevel : byte
    {
        Private,
        Protected,
        Public
    }

    /// <summary>
    /// 定義
    /// </summary>
    public static class Define
    {
        #region 定義
        /// <summary>
        /// VisualStudio2022のインストールディレクトリ
        /// </summary>
        public const string VisualStudioDirectory = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\";

        /// <summary>
        /// MsBuild.exeのパス
        /// </summary>
        public const string MsBuildExePath = VisualStudioDirectory + "Msbuild\\Current\\Bin\\MSBuild.exe";

        public const string NitroReflectionType = "reflection::Type";
        #endregion
    }
}
