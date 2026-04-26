using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
    [Core.Attributes.RuntimeWrapper("nox::Node")]
    public class Node : Core.RuntimeWrapper.ManagedObject
    {
        public Node? ParentNode { get; set; } = null;

        #region 公開メソッド
        public Node()
        {

        }
        #endregion
    }
}
