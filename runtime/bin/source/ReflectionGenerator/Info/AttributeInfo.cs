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
        public required string FullName { get; init; } 

        /// <summary>
        /// コンストラクタの引数
        /// </summary>
        public string ArgsStr { get; init; } = string.Empty;

        public required bool IsConstexpr { get; init; }
    }

    public class AttributeInfoStack : IStacker<AttributeInfo>
    {

    }
}
