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

    public static class HashSetExtension
    {
        #region 公開メソッド
        public static bool TryAdd<T>(this HashSet<T> hashSet, T val)
        {
            if (!hashSet.Contains(val))
            {
                hashSet.Add(val);
                return true;
            }
            return false;
        }

        #endregion
    }

    public static class IReadOnlyListExtension
    {
        public static void ForEach<T>(this IReadOnlyList<T> self, Action<T> action)
        {
            int length = self.Count;
            for(int i = 0; i < length; i++)
            {
                action(self[i]);
            }
        }
    }
}
