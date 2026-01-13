using Core.Attributes;
using System.Runtime.CompilerServices;

namespace Core
{

	[Core.Attributes.RuntimeWrapper("nox::Object")]
	public class RuntimeObject
	{
		#region フィールド
		/// <summary>
		/// メンバ変数リスト
		/// </summary>
		private readonly object[] _VariableList;
		private readonly bool[] _VariableDirtyList;

		/// <summary>
		/// メンバ関数リスト
		/// get,set
		/// </summary>
		private readonly object[] _PropertyFunctionList;
		#endregion

		#region 公開プロパティ
		public Core.RuntimeRecordDecl RuntimeRecordDecl { get; init; }
		#endregion

		#region 公開メソッド
		protected RuntimeObject(Core.RuntimeRecordDecl runtimeRecordDecl)
		{
			RuntimeRecordDecl = runtimeRecordDecl;

			_PropertyFunctionList = [];

			ReadOnlySpan<RuntimeVariableDecl> variableList = RuntimeRecordDecl.VariableList;
			int variableListLength = variableList.Length;
			_VariableList = new object[variableListLength];
			_VariableDirtyList = new bool[variableListLength];

			for (int i = 0; i < variableListLength; ++i)
			{
				_VariableList[i] = CreateRuntimeVariable(variableList[i]);
				_VariableDirtyList[i] = false;
			}
		}
		#endregion

		#region 非公開メソッド
		private static object CreateRuntimeVariable(RuntimeTypeKind kind)
		{
			switch (kind)
			{
				case RuntimeTypeKind.Bool:
					return (bool)default;
				case RuntimeTypeKind.Int8:
					return (sbyte)default;
				case RuntimeTypeKind.UInt8:
					return (byte)default;
				case RuntimeTypeKind.Int16:
					return (short)default;
				case RuntimeTypeKind.Uint16:
					return (ushort)default;
				case RuntimeTypeKind.Int32:
					return (int)default;
				case RuntimeTypeKind.UInt32:
					return (uint)default;
				case RuntimeTypeKind.Int64:
					return (long)default;
				case RuntimeTypeKind.UInt64:
					return (ulong)default;
				case RuntimeTypeKind.Float:
					return (float)default;
				case RuntimeTypeKind.Double:
					return (double)default;
			}
			return null!;
		}

		private static object CreateRuntimeVariable(RuntimeVariableDecl variableDecl)
		{
			switch (variableDecl.TypeInfo.TypeKind)
			{
				case RuntimeTypeKind.Enum:
					return CreateRuntimeVariable(variableDecl.TypeInfo.TypeKind);

				case RuntimeTypeKind.Class:
				case RuntimeTypeKind.Struct:
				case RuntimeTypeKind.Union:

					break;

				default:
					return CreateRuntimeVariable(variableDecl.TypeInfo.TypeKind);
			}

			return null!;
		}
		#endregion
	}

	public interface IRuntimeObject<T> where T : Core.RuntimeObject, IRuntimeObject<T> 
	{
		private static RuntimeRecordDecl? _RuntimeRecordDecl = null;
		public static RuntimeRecordDecl RuntimeRecordDecl
		{
			get
			{
				Nox.Util.Assert(_RuntimeRecordDecl != null, "_RuntimeRecordDecl is null");
				return _RuntimeRecordDecl;
			}
		}
	}
}
