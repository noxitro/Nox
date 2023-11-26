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
            public string Name { get; set; } = string.Empty;

            /// <summary>
            /// 型名
            /// </summary>
            public string TypeName { get; set; } = string.Empty;

            public string TypeNamespace { get; set; } = string.Empty;

            /// <summary>
            /// 型
            /// </summary>
            public ClangSharp.Interop.CXTypeKind CXTypeKind { get; set; } = ClangSharp.Interop.CXTypeKind.CXType_Invalid;

            /// <summary>
            /// デフォルト値を持つか
            /// </summary>
            public bool HasDefaultValue { get; set; } = false;
        }



        public string Name { get; set; } = string.Empty;
        public string FullName { get; set; } = string.Empty;

        /// <summary>
        /// 関数の型名
        /// </summary>
        public string MethodTypeName { get; set; } = string.Empty;

        private MethodAttributeFlag _MethodAttributeFlags = MethodAttributeFlag.None;

        public bool IsVirtual
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public bool IsAbstract
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Abstract);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Abstract, value);
        }

        /// <summary>
        /// Constructorか
        /// </summary>
        public bool IsConstructor
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Constructor);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Constructor, value);
        }

        public bool IsTemplate
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public bool IsInline
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public bool IsConstexpr
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public bool IsConst
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public bool IsNoexcept
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }
        public bool IsStatic
        {
            get => _MethodAttributeFlags.HasFlag(MethodAttributeFlag.Virtual);
            set => _MethodAttributeFlags = (MethodAttributeFlag)Util.SetBit((uint)_MethodAttributeFlags, (uint)MethodAttributeFlag.Virtual, value);
        }

        /// <summary>
        /// 戻り値の型
        /// </summary>
        public RuntimeType ReturnRuntimeType { get; set; } = new RuntimeType();

        /// <summary>
        /// アクセスレベル
        /// </summary>
        public AccessLevel AccessLevel { get; set; } = AccessLevel.Private;

        /// <summary>
        /// 引数情報
        /// </summary>
        public ArgInfo[] ArgInfoList { get; set; } = new ArgInfo[0];

        /// <summary>
        /// 属性リスト
        /// </summary>
        public List<AttributeInfo> AttributeInfoList { get; set; } = new List<AttributeInfo>();
    }
}