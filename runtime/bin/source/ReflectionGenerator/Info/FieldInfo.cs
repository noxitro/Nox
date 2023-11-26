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
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// 完全な変数名
        /// </summary>
        public string FullName { get; set; } = string.Empty;

        /// <summary>
        /// ランタイムタイプ
        /// </summary>
        public RuntimeTypeKind RuntimeType { get; set; } = RuntimeTypeKind.S32;

        /// <summary>
        /// ランタイムタイプ属性
        /// </summary>
        public RuntimeAttribute RuntimeAttributes { get; set; } = RuntimeAttribute.Invalid;

        /// <summary>
        /// アクセスレベル
        /// </summary>
        public AccessLevel AccessLevel { get; set; } = AccessLevel.Private;

        /// <summary>
        /// IsConstexpr
        /// </summary>
        public bool IsConstexpr { get; set; } = false;

        /// <summary>
        /// Templateか
        /// </summary>
        public bool IsTemplate { get; set; } = false;

        /// <summary>
        /// 属性リスト
        /// </summary>
        public List<AttributeInfo> AttributeInfoList { get; set; } = new List<AttributeInfo>();
    }
}
