using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public enum TypeInfoKind : byte
    {
        Enum,
        Class,
        Union,
        Type,
        Variable,
        Function,
        GlobalDecl,
    }

    /// <summary>
    /// モジュール情報
    /// </summary>
    public struct Module : IEquatable<Module>
    {
        /// <summary>
        /// モジュール名(プロジェクト名)
        /// core-> "core"
        /// module/render -> "module_render"
        /// </summary>
        public readonly required string ModuleName { get; init; } 

        readonly bool IEquatable<Module>.Equals(ReflectionGenerator.Info.Module other)
        {
            return ModuleName == other.ModuleName;
        }

        public static readonly Module Invalid = new Module { ModuleName = "Invalid" };
    }

    public interface IBaseInfo
    {
        public Module Module { get; }
        public TypeInfoKind TypeInfoKind { get; }
    }

    /// <summary>
    /// パース情報基底
    /// </summary>
    public abstract class BaseInfo : IBaseInfo
    {
        public required Module Module { get; init; }

        public abstract TypeInfoKind TypeInfoKind { get; }

        public bool IsReflection { get; init; }
    }
}
