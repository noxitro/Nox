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
        /// 属性文字列
        /// </summary>
        public required string ValueStr { get; init; }

        /// <summary>
        /// 属性クラス情報
        /// </summary>
        public required ClassInfo AttributeClassInfo { get; init; }

        /// <summary>
        /// ConstexprなConstructorかどうか
        /// </summary>
        public required bool IsConstexpr { get; init; }
    }

    public class AttributeInfoStack : IStacker<AttributeInfo>
    {

    }
}
