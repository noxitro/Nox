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
    public abstract class AttributeInfo 
    {
        /// <summary>
        /// 属性の種類
        /// </summary>
        public required ClangSharp.Interop.CX_AttrKind AttrKind { get; init; }

        public string Hash { get; }

        private static int _IndexCounter = 0;

        public AttributeInfo()
        {
            Hash = System.Threading.Interlocked.Increment(ref _IndexCounter).ToString();

		}
	}

	/// <summary>
	/// 標準属性情報
	/// </summary>
	public class StandardAttribute : AttributeInfo
    {

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
