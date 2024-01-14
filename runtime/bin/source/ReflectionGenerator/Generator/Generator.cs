using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Generator
{
    /// <summary>
    /// リフレクションコード生成
    /// </summary>
    public class Generator
    {
        #region 非公開フィールド
        private string _BaseDirectory = string.Empty;

        /// <summary>
        /// baseファイルに定義する関数名リスト
        /// </summary>
        private List<string> _BaseFileMethodNameList = new List<string>();

        #endregion

        #region 公開プロパティ
        public required Parser.CppParser Parser { private get; init; }

        /// <summary>
        /// 1ファイルに定義する数
        /// </summary>
        public uint DevisionInfoCount { private get; init; } = 10;

        public required string OutputDirectory { private get; init; }
        public required string BuildSpec { private get; init; }
        public required string Platform { private get; init; }
        public required string BuildSpecDefine { private get; init; }
        public required string PlatformDefine { private get; init; }
        #endregion

        #region 公開メソッド
        public void Init()
        {
            _BaseDirectory = Path.GetFullPath($"{OutputDirectory}/{BuildSpec}/{Platform}/");
        }

        public bool Generate()
        {
            if (Directory.Exists(_BaseDirectory) == false)
            {
                Trace.Error("存在しないディレクトリです:{0}", _BaseDirectory);
                return false;
            }

            return GenerateImpl();
        }
        #endregion

        #region 非公開メソッド
        private bool GenerateImpl()
        {
            //  モジュールごとのフォルダを作成
            foreach (string moduleName in Parser.ModuleNameList)
            {

            }

                return true;
        }
        #endregion
    }
}
