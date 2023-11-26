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
            public string Name { get; set; } = string.Empty;

            /// <summary>
            /// 
            /// </summary>
            public ulong UnsinedValue { get; set; } = 0U;

            /// <summary>
            /// 
            /// </summary>
            public long SinedValue { get; set; } = 0;

            /// <summary>
            /// 属性リスト
            /// </summary>
            public List<AttributeInfo> AttributeInfoList { get; set; } = new List<AttributeInfo>();
            #endregion
        }


        #region 公開プロパティ
        /// <summary>
        /// フルネーム
        /// </summary>
        public string FullName { get; set; } = string.Empty;

        /// <summary>
        /// 名前
        /// </summary>
        public string Name { set; get; } = string.Empty;

        /// <summary>
        /// 自身が含まれる外部クラス
        /// </summary>
        public ClassInfo? OutsideClass { get; set; } = null;


        public AccessLevel AccessLevel { get; set; } = AccessLevel.Private;

        /// <summary>
        /// 基底型
        /// </summary>
        public RuntimeTypeKind UnderlyingType { get; set; } = RuntimeTypeKind.Invalid;

        /// <summary>
        /// 要素リスト
        /// 定義順
        /// </summary>
        public List<EnumVariable> VariableList { get; set; } = new List<EnumVariable>();

        /// <summary>
        /// 符号なし型
        /// </summary>
        public bool IsUnsigned { get; set; } = false;

        public List<AttributeInfo> AttributeInfoList { get; set; } = new List<AttributeInfo>();
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
