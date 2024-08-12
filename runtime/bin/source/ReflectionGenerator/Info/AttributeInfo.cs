using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    /// <summary>
    /// 属性情報
    /// </summary>
    public class AttributeInfo 
    {
        /// <summary>
        /// 属性の種類
        /// </summary>
        public required ClangSharp.Interop.CX_AttrKind AttrKind { get; init; }
    }

    /// <summary>
    /// UserCustomAttribute情報
    /// </summary>
    public class EngineAnnotateAttribute : AttributeInfo
    {
        /// <summary>
        /// 属性文字列
        /// </summary>
        public required string Value { get; init; }

        /// <summary>
        /// コンパイル時定数かどうか
        /// 未実装
        /// </summary>
        public required bool IsConstexpr { get; init; }
    }
}
