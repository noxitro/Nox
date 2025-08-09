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

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Explicit)]
    public struct Integer64
    {
        [System.Runtime.InteropServices.FieldOffset(0)]
        public long Int64;

        [System.Runtime.InteropServices.FieldOffset(0)]
        public ulong UInt64;
    }

    /// <summary>
    /// 列挙体情報
    /// </summary>
    public class EnumInfo : Info.TypeInfo
    {
        /// <summary>
        /// Enumの1要素
        /// </summary>
        public struct EnumVariable
        {
            #region 公開プロパティ
            public required ClangSharp.Interop.CXTypeKind TypeKind { get; init; }

            /// <summary>
            /// 名前
            /// </summary>
            public required string Name { get; init; }

            public required bool IsUnsigned { get; init; }

			public required Integer64 Integer64 { get; init; }

            /// <summary>
            /// 属性リスト
            /// </summary>
            public required IReadOnlyList<AttributeInfo> AttributeInfoList { get; init; }
            #endregion
        }

        #region 公開メソッド
        public EnumInfo(ReadOnlySpan<EnumVariable> variableList)
        {
            _VariableList = variableList.ToArray();
		}
        #endregion

        #region 公開プロパティ
        /// <summary>
        /// 要素リスト
        /// 定義順
        /// </summary>
        public readonly EnumVariable[] _VariableList;


		public ReadOnlySpan<EnumVariable> VariableList => _VariableList;
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
