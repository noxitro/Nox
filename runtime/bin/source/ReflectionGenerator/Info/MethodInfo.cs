using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public class MethodInfo : BaseInfo
    {
        private enum MethodAttributeFlag : uint
        {
            None = 0,
            Virtual = 1 << 0,
            Abstract = 1 << 1,
            Constructor = 1 << 2,
            Constexpr = 1 << 3,
            Template = 1 << 4,
            Inline = 1 << 5,
            Const = 1 << 6,
            Noexcept = 1 << 7,
            Static = 1 << 8,
        }

        /// <summary>
        /// 引数情報
        /// </summary>
        public class ArgInfo
        {
            /// <summary>
            /// 引数名
            /// </summary>
            public required string Name { get; init; }

            public required RuntimeType RuntimeType { get; init; }

            /// <summary>
            /// 型
            /// </summary>
            public required ClangSharp.Interop.CXTypeKind CXTypeKind { get; init; }

            /// <summary>
            /// デフォルト値を持つか
            /// </summary>
            public required bool HasDefaultValue { get; init; }
        }

        public required string Name { get; init; }

        private MethodAttributeFlag _MethodAttributeFlags = MethodAttributeFlag.None;

        public required bool IsVirtual
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public required bool IsAbstract
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Abstract);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Abstract, value);
        }

        /// <summary>
        /// Constructorか
        /// </summary>
        public required bool IsConstructor
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Constructor);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Constructor, value);
        }

        public bool IsTemplate
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public required bool IsInline
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public required bool IsConstexpr
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public required bool IsConst
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public required bool IsNoexcept
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public required bool IsStatic
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }

        /// <summary>
        /// 戻り値の型
        /// </summary>
        public required RuntimeType ReturnRuntimeType { get; init; }

        /// <summary>
        /// アクセスレベル
        /// </summary>
        public required AccessLevel AccessLevel { get; init; }

        /// <summary>
        /// 引数情報
        /// </summary>
        public required IReadOnlyList<ArgInfo> ArgInfoList { get; init; }

        /// <summary>
        /// 属性リスト
        /// </summary>
        public required IReadOnlyList<AttributeInfo> AttributeInfoList { get; init; }
    }
}