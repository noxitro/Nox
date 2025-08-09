using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    /// <summary>
    /// 関数情報
    /// </summary>
    public class FunctionInfo : Info.BaseInfo
    {
        public readonly struct ArgumentInfo
        {
            public required string Name { get; init; }
            public required bool IsDefault { get; init; }
            public required string TypeFullName { get; init; }
		}

        public override TypeInfoKind TypeInfoKind => TypeInfoKind.Function;
        public required string Name { get; init; }
        public required string FullName { get; init; }
        public required string Namespace { get; init; }

		public required AccessLevel AccessLevel { get; init; }

		/// <summary>
		/// 型名
		/// </summary>
		public required string FunctionTypeFullName { get; init; }

        public required uint NumArguments { get; init; }

        public required uint NumDefaultArguments { get; init; }

        public override string ToString() => FullName;

        public required bool IsInline { get; init; }
        public required bool IsConstexpr { get; init; }
		public required bool IsVirtual { get; init; }
        public required bool IsPureVirtual { get; init; }
        public required bool IsConsteval { get; init; }
        public bool IsConstructor => IsDefaultConstructor || IsCopyConstructor || IsMoveConstructor;
		public required bool IsDefaultConstructor { get; init; }
        public required bool IsCopyConstructor { get; init; }
        public required bool IsMoveConstructor { get; init; }
        public required bool IsDestructor { get; init; }
		public virtual bool IsTemplated { get; } = false;

        public required ClangSharp.Interop.CX_OverloadedOperatorKind OperatorKind { get; init; }

        public required ArgumentInfo[] ArgumentInfoList { get; init; }
        public required bool IsNoReturn { get; init; }
        public required bool IsNoexcept { get; init; }
	}

    public class TemplateFunctionInfo : FunctionInfo
    {
        /// <summary>
        /// 特殊化名リスト
        /// </summary>
        public required IReadOnlyList<string> SpecializationsList { get; init; }

        public override bool IsTemplated { get; } = true;
    }
}
