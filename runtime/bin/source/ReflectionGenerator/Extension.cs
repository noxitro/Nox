using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    public static class Extension
    {
        #region 公開メソッド
        public static string ToCppString(this AccessLevel accessLevel)
        {
            return $"{nameof(AccessLevel)}::{accessLevel.ToString()}";
        }

        public static string ToLowerString(this bool val) => val ? "true" : "false";
        #endregion
    }
}
