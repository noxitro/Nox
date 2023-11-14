using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Parser
{
    public class CppParser
    {
        #region 公開定義
        public class SetupParam
        {
            public CppParseDefine.CppVersion CppVersion { get; set; } = CppParseDefine.CppVersion.LastTest;
            public string SourceFilePath { get; set; } = string.Empty;
        }
        #endregion

        #region 公開メソッド
        public bool Parse(SetupParam setupParam)
        {
            return true;
        }
        #endregion
    }
}
