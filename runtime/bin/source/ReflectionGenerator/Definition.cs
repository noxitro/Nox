using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    public class RuntimeType
    {
        public RuntimeTypeKind Kind { get; set; } = RuntimeTypeKind.Invalid;

        public string Name { get; set; }    = string.Empty;
        public string FullName { get; set; } = string.Empty;

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
