using Core.Attributes;
using Nox.Extensions;

namespace Core
{

	[Core.Attributes.RuntimeWrapper("nox::Object")]
	public abstract class RuntimeObject
	{
		#region フィールド
		private long _RemoteInstanceId = 0;

		/// <summary>
		/// メンバ変数リスト
		/// </summary>
		private readonly object?[] _VariableList;
		private readonly bool[] _VariableDirtyList;

		/// <summary>
		/// c++の関数だが、プロパティとして扱うもののリスト
		/// </summary>
		private readonly object[] _PropertyFunctionList;

		#endregion

		#region 公開プロパティ
		public Core.RuntimeRecordDecl RuntimeRecordDecl { get; init; }
		protected abstract RuntimeRecordDecl GetRuntimeRecordDecl();
		#endregion

		#region 公開メソッド
		protected RuntimeObject()
		{
			//
			RuntimeRecordDecl = GetRuntimeRecordDecl();

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

			//	プロパティとして扱う関数の収集
			ReadOnlySpan<RuntimeFunctionDecl> functionList = RuntimeRecordDecl.FunctionList;
			foreach (var function in functionList)
			{
			}
		}

		public object? GetValue(string name)
		{
			var index = RuntimeRecordDecl.VariableList.FindIndex(x => x.Name == name);
			if (index <= -1)
			{
				Nox.Util.Assert(false, $"変数 '{name}' は存在しません。");
			}

			return _VariableList[index];
		}

		public void SetValue(string name, object? value)
		{
			var index = RuntimeRecordDecl.VariableList.FindIndex(x => x.Name == name);
			if (index <= -1)
			{
				Nox.Util.Assert(false, $"変数 '{name}' は存在しません。");
			}
			_VariableList[index] = value;
			_VariableDirtyList[index] = true;
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

		protected T Get<T>([System.Runtime.CompilerServices.CallerMemberName] string propertyName = "")
		{
			object? value = GetValue(propertyName);
			Nox.Util.Assert	(value is T, $"プロパティ '{propertyName}' の型が '{typeof(T).FullName}' ではありません。");
			return (T)value;
		}

		protected void Set<T>(T value, [System.Runtime.CompilerServices.CallerMemberName] string propertyName = "")
		{
			SetValue(propertyName, value);
		}
		#endregion
	}

	/// <summary>
	/// 型情報を保持するためのインターフェース
	/// </summary>
	/// <typeparam name="T"></typeparam>
	public interface IRuntimeObject<T> where T : Core.RuntimeObject, IRuntimeObject<T> 
	{
		static abstract RuntimeRecordDecl StaticRuntimeRecordDecl { get; set; }
		static RuntimeRecordDecl GetRuntimeRecordDecl() => T.StaticRuntimeRecordDecl;
	}
}
