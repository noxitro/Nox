using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public class VariableInfo
    {

    }

    /// <summary>
    /// 変数情報
    /// </summary>
    public class FieldInfo : BaseInfo
    {
        /// <summary>
        /// 変数名
        /// </summary>
        public required string Name { get; init; }

        /// <summary>
        /// スコープ名を含めた名前
        /// </summary>
        public required string FullName { get; init; }

        /// <summary>
        /// 型情報
        /// </summary>
        public required RuntimeType RuntimeType { get; init; }

        /// <summary>
        /// アクセスレベル
        /// </summary>
        public required AccessLevel AccessLevel { get; init; }

        /// <summary>
        /// IsConstexpr
        /// </summary>
        public required bool IsConstexpr { get; init; }

        /// <summary>
        /// Templateか
        /// </summary>
        public required bool IsTemplate { get; init; } 

        /// <summary>
        /// 属性リスト
        /// </summary>
        public required IReadOnlyList<AttributeInfo> AttributeInfoList { get; init; }
    }
}
