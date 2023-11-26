using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    /// <summary>
    /// テンプレート引数の種別
    /// </summary>
    public enum TemplateParamType : byte
    {
        Tempalte,
        TemplateTempalte,
        NonType,
    }

    public struct TemplateParam
    {
        /// <summary>
        /// 種別
        /// </summary>
        public required TemplateParamType Kind { get; init; }
    }

    /// <summary>
    /// クラス情報
    /// </summary>
    public class ClassInfo : BaseInfo
    {
        public enum AttributeFlag : uint
        {
            None = 0,
            RootClass = 1 << 0,
            Template = 1 << 1,
            Attribute = 1 << 2,
            NitroObjectClass = 1 << 3,
            Class = 1 << 4,
            Union = 1 << 5,
            Reflection = 1 << 6,
            Typedef = 1 << 7,
        }

        #region 公開プロパティ
        public string ParentObjectClassName { get; set; } = string.Empty;

        /// <summary>
        /// 親クラス
        /// </summary>
        public ClassInfo? ParentObjectClass { get; set; } = null;

        /// <summary>
        /// 自身が含まれる外部クラス
        /// </summary>
        public ClassInfo? OutsideClass { get; set; } = null;

        /// <summary>
        /// テンプレートパラメータリストテーブル
        /// </summary>
        public TemplateParam[][] TemplateParamListTable { get; init; } = new TemplateParam[0][];

        public required ClangSharp.Interop.CXCursor CXCursor { get; init; }

        public int DirtyResolveBaseInfo = 0;

        public string[] BaseClassNameList { get; set; } = new string[0];

        /// <summary>
        /// 継承型リスト
        /// </summary>
        public List<ClassInfo> BaseClassList { get; set; } = new List<ClassInfo>();

        public string RecordClassFullName { get; set; } = string.Empty;

        /// <summary>
        /// 別名定義の元クラス情報
        /// </summary>
        public ClassInfo? RecordClass { get; set; } = null;

        /// <summary>
        /// 別名定義が存在する数
        /// </summary>
        public int TypedefCount = 0;

        /// <summary>
        /// 別名定義のID
        /// note:   もう少しいい方法がないか
        /// </summary>
        public int TypedefID { get; set; } = 0;

        /// <summary>
        /// リフレクション対応クラスか
        /// falseの場合、publicのみリフレクション対応になる
        /// </summary>
        public bool IsReflection
        {
            get => AttributeFlags.HasFlag(AttributeFlag.Reflection);
            set => AttributeFlags = (AttributeFlag)Util.SetBit((uint)AttributeFlags, (uint)AttributeFlag.Reflection, value);
        }

        /// <summary>
        /// 別名定義クラス
        /// </summary>
        public bool IsTypedef
        {
            get => AttributeFlags.HasFlag(AttributeFlag.Typedef);
            set => AttributeFlags = (AttributeFlag)Util.SetBit((uint)AttributeFlags, (uint)AttributeFlag.Typedef, value);
        }

        /// <summary>
        /// クラス名
        /// </summary>
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// スコープを含めたクラス名
        /// </summary>
        public string FullName { get; set; } = string.Empty;

        /// <summary>
        /// アクセスレベル
        /// </summary>
        public AccessLevel AccessLevel { get; set; } = AccessLevel.Private;

        public AttributeFlag AttributeFlags { get; set; } = AttributeFlag.None;

        /// <summary>
        /// class
        /// </summary>
        public bool IsClass
        {
            get => AttributeFlags.HasFlag(AttributeFlag.Class);
            set => AttributeFlags = (AttributeFlag)Util.SetBit((uint)AttributeFlags, (uint)AttributeFlag.Class, value);
        }

        /// <summary>
        /// union
        /// </summary>
        public bool IsUnion
        {
            get => AttributeFlags.HasFlag(AttributeFlag.Union);
            set => AttributeFlags = (AttributeFlag)Util.SetBit((uint)AttributeFlags, (uint)AttributeFlag.Union, value);
        }

        /// <summary>
        /// テンプレート
        /// </summary>
        public bool IsTemplate
        {
            get => AttributeFlags.HasFlag(AttributeFlag.Template);
            set => AttributeFlags = (AttributeFlag)Util.SetBit((uint)AttributeFlags, (uint)AttributeFlag.Template, value);
        }

        /// <summary>
        /// 継承型
        /// </summary>
        public bool IsAttribute
        {
            get => AttributeFlags.HasFlag(AttributeFlag.Attribute);
            set => AttributeFlags = (AttributeFlag)Util.SetBit((uint)AttributeFlags, (uint)AttributeFlag.Attribute, value);
        }

        /// <summary>
        /// ルートクラスかどうか
        /// </summary>
        public bool IsRootClass
        {
            get => AttributeFlags.HasFlag(AttributeFlag.RootClass);
            set => AttributeFlags = (AttributeFlag)Util.SetBit((uint)AttributeFlags, (uint)AttributeFlag.RootClass, value);
        }

        /// <summary>
        /// ::nitroクラス
        /// </summary>
        public bool IsNitroObjectClass
        {
            get => AttributeFlags.HasFlag(AttributeFlag.NitroObjectClass);
            set => AttributeFlags = (AttributeFlag)Util.SetBit((uint)AttributeFlags, (uint)AttributeFlag.NitroObjectClass, value);
        }

        /// <summary>
        /// 属性リスト
        /// </summary>
        public required IReadOnlyList<AttributeInfo> AttributeInfoList { get; init; } 

        /// <summary>
        /// 関数リスト
        /// </summary>
        public List<MethodInfo> MethodInfoList { get; set; } = new List<MethodInfo>();

        /// <summary>
        /// 変数リスト
        /// </summary>
        public List<FieldInfo> FieldInfoList { get; set; } = new List<FieldInfo>();
        #endregion
    }

    /// <summary>
    /// クラススタック
    /// </summary>
    public class ClassInfoStack : IStacker<ClassInfo>
    {
        #region 非公開フィールド
        /// <summary>
        /// クラス名に依る辞書
        /// </summary>
        private Dictionary<string, ClassInfo> _InfoDic = new Dictionary<string, ClassInfo>();

        /// <summary>
        /// CXCursorに依る辞書
        /// </summary>
        private readonly Dictionary<ClangSharp.Interop.CXCursor, ClassInfo> _InfoDictFromCXCursor = new Dictionary<ClangSharp.Interop.CXCursor, ClassInfo>();

        private List<ClassInfo> _TypedefClassInfoList = new List<ClassInfo>();
        #endregion

        public override void Push(ClassInfo info)
        {
            base.Push(info);

            if(_InfoDic.ContainsKey(info.FullName) == true)
            {
                Trace.Error(this, string.Format("既に存在しています {0}:", info.FullName));
                return;
            }

            _InfoDic.Add(info.FullName, info);

            System.Diagnostics.Debug.Assert(_InfoDictFromCXCursor.ContainsKey(info.CXCursor) == false, $"既に登録されています:{info.FullName}");
            _InfoDictFromCXCursor.Add(info.CXCursor, info);

            if (info.IsTypedef == true)
            {
                _TypedefClassInfoList.Add(info);
            }
        }

        public bool Contains(string fullName) => _InfoDic.ContainsKey(fullName);

        public ClassInfo? GetInfo(string fullName)
        {
            if(_InfoDic.ContainsKey(fullName) == false)
            {
                return null;
            }

            return _InfoDic[fullName];
        }

        public ClassInfo? GetInfo(ClangSharp.Interop.CXCursor cursor)
        {
            _InfoDictFromCXCursor.TryGetValue(cursor, out ClassInfo? info);
            return info;
        }

        public List<ClassInfo> GetTypedefClassInfoList() => _TypedefClassInfoList;
    }

 
}
