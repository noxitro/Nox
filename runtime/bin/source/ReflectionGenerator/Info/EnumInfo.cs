using ClangSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public class EnumInfoGenerator
    {

    }

    /// <summary>
    /// 列挙体情報
    /// </summary>
    public class EnumInfo : BaseInfo
    {
        /// <summary>
        /// Enumの1要素
        /// </summary>
        public class EnumVariable
        {
            #region 公開プロパティ
            /// <summary>
            /// 名前
            /// </summary>
            public required string Name { get; init; }

            /// <summary>
            /// 属性リスト
            /// </summary>
            public required IReadOnlyList<AttributeInfo> AttributeInfoList { get; init; }
            #endregion
        }


        #region 公開プロパティ
        /// <summary>
        /// フルネーム
        /// </summary>
        public required string FullName { get; init; }

        /// <summary>
        /// 名前
        /// </summary>
        public required string Name { get; init; }

        public required string Namespace { get; init; }

        public required AccessLevel AccessLevel { get; init; }

        /// <summary>
        /// 要素リスト
        /// 定義順
        /// </summary>
        public required IReadOnlyList<EnumVariable> VariableList { get; init; }

        public required IReadOnlyList<AttributeInfo> AttributeInfoList { get; init; }
        #endregion
    }

    /// <summary>
    /// Enum
    /// </summary>
    public class EnumInfoStack : IStacker<EnumInfo>
    {
        #region 公開プロパティ

        #endregion

        #region 公開メソッド
     
        #endregion
    }
}
