using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    public static class Util
    {
        #region 公開メソッド
        public static uint BitOr(uint a, uint b) => a | b;
        public static uint BitXor(uint a, uint b) => a & ~b;

        public static uint SetBit(uint thisBits, uint bits, bool flag)  => flag == true ? BitOr(thisBits, bits) : BitXor(thisBits, bits);

        public static string GetRuntimeRootNamespaceScope() => Define.RUNTIME_ROOT_NAMESPACE_STR;
        public static string GetRuntimeReflectionNamespaceScope() => $"{Define.RUNTIME_ROOT_NAMESPACE_STR}::{Define.RUNTIME_REFLECTION_NAMESPACE_STR}";
        #endregion
    }
}
