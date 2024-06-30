using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    /// <summary>
    /// paser用のstack,list管理
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class IStacker<T> where T : class
    {
        #region 公開プロパティ
        public T Current => _Stack.Peek();
        public bool EmptyStack => _Stack.Count <= 0;

        public IReadOnlyList<T> InfoList => _List;
        public IReadOnlyCollection<T> InfoStack => _Stack;

        public T Top => _List[0];
        #endregion

        #region 公開メソッド
        /// <summary>
        /// 要素の追加
        /// </summary>
        /// <param name="info"></param>
        public virtual void Push(T info)
        {
            _Stack.Push(info);
            _List.Add(info);
        }

        /// <summary>
        /// 要素の破棄
        /// </summary>
        public void Pop()
        {
            _Stack.Pop();
        }

        public void Clear()
        {
            _Stack.Clear();
            _List.Clear();
        }
        #endregion

        #region 非公開フィールド
        protected readonly List<T> _List = new List<T>();
        protected readonly Stack<T> _Stack = new Stack<T>();
        #endregion
    }
}
