namespace Nox
{
	public class RuntimeObjectWrapperAttribute : System.Attribute
	{
		private readonly string _Fqn;
		public ReadOnlySpan<char> Fqn => _Fqn;

        public RuntimeObjectWrapperAttribute(string fqn)
		{
			_Fqn = fqn;
        }
    }

    public abstract class RuntimeObject
	{
		#region 公開メソッド
		public abstract RuntimeType GetTypeInfo();
		#endregion

		#region 非公開フィールド
		private byte[] _Properties = Array.Empty<byte>();
        #endregion
    }

    public abstract class RuntimeObject<T> : RuntimeObject where T : RuntimeObject<T>
    {
		#region 非公開フィールド
		public static RuntimeType TypeInfo { get; } = new RuntimeType();
		#endregion

		#region 公開メソッド
		public override sealed RuntimeType GetTypeInfo() => TypeInfo;
        #endregion
    }

	[RuntimeObjectWrapper("nox::GameObject")]
    public class GameObject : RuntimeObject<GameObject>
    {

	}
}
