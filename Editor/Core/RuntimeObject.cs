// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using Core.Attributes;
using Nox.Extensions;
using System.Numerics;
using System.Reflection;

namespace Core;


	[Core.Attributes.RuntimeWrapper("nox::Object")]
	public abstract class RuntimeObject
{
    internal static class RuntimeRecordDeclHolder<T> where T : RuntimeObject
    {
        public static RuntimeRecordDecl Value = null!;
    }
    #region フィールド
    /// <summary>
    /// メンバ変数リスト
    /// </summary>
		private readonly object[] _VariableList;
		private readonly bool[] _VariableDirtyList;
		private readonly string _RuntimeFqn;

		/// <summary>
		/// c++の関数だが、プロパティとして扱うもののリスト
		/// </summary>
		private readonly object[] _PropertyValueList;
		private readonly bool[] _PropertyDirtyList;

		#endregion

		#region 公開プロパティ
		public Core.RuntimeRecordDecl RuntimeRecordDecl { get; init; }
		public long RemoteInstanceId { get; set; } = 0;
		public Core.Net.RuntimeRemoteClient? RemoteClient { get; private set; }
		public ReadOnlySpan<object> VariableList => _VariableList;
		public ReadOnlySpan<object> PropertyValueList => _PropertyValueList;
		public string RuntimeFqn => _RuntimeFqn;

		public Span<object> RefVariableList => _VariableList;
		public Span<object> RefPropertyValueList => _PropertyValueList;
		public ReadOnlySpan<bool> VariableDirtyList => _VariableDirtyList;
		public ReadOnlySpan<bool> PropertyDirtyList => _PropertyDirtyList;
		#endregion

		#region 公開メソッド
		protected RuntimeObject()
		{
			{
				var holderType = typeof(RuntimeRecordDeclHolder<>).MakeGenericType(GetType());
				var decl = holderType.GetField("Value")!.GetValue(null) as Core.RuntimeRecordDecl;
				Nox.Util.Assert(decl != null,
					$"型 '{GetType().FullName}' の RuntimeRecordDecl が設定されていません。" +
					$"RuntimeWrapperAttribute が付与されているか確認してください。");
				RuntimeRecordDecl = decl;
			}
			_RuntimeFqn = GetRuntimeFqn(GetType());

			(_VariableList, _VariableDirtyList, _PropertyValueList, _PropertyDirtyList) = CreateRuntimeStorage(RuntimeRecordDecl);
		}

		protected RuntimeObject(RuntimeRecordDecl runtimeRecordDecl)
		{
			RuntimeRecordDecl = runtimeRecordDecl;
			_RuntimeFqn = runtimeRecordDecl.FullName;
			(_VariableList, _VariableDirtyList, _PropertyValueList, _PropertyDirtyList) = CreateRuntimeStorage(RuntimeRecordDecl);
		}

		private static (object[] Variables, bool[] VariableDirtyFlags, object[] PropertyValues, bool[] PropertyDirtyFlags) CreateRuntimeStorage(RuntimeRecordDecl runtimeRecordDecl)
		{
			ReadOnlySpan<RuntimeVariableDecl> variableList = runtimeRecordDecl.VariableList;
			ReadOnlySpan<RuntimePropertyDecl> propertyList = runtimeRecordDecl.PropertyList;
			int variableListLength = variableList.Length;
			int propertyListLength = propertyList.Length;
			object[] variableValueList = new object[variableListLength];
			bool[] variableDirtyList = new bool[variableListLength];
			object[] propertyValueList = new object[propertyListLength];
			bool[] propertyDirtyList = new bool[propertyListLength];


			for (int i = 0; i < variableListLength; ++i)
			{
				variableValueList[i] = CreateRuntimeVariable(variableList[i]);
				variableDirtyList[i] = false;
			}

			for (int i = 0; i < propertyListLength; ++i)
			{
				propertyValueList[i] = CreateRuntimeValue(propertyList[i].TypeInfo);
				propertyDirtyList[i] = false;
			}

			return (variableValueList, variableDirtyList, propertyValueList, propertyDirtyList);
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
			SetMatchingPropertyValueFromVariable(name, value, isDirty: true);
		}

		public bool IsDirty(string name)
		{
			var index = RuntimeRecordDecl.VariableList.FindIndex(x => x.Name == name);
			if (index <= -1)
			{
				Nox.Util.Assert(false, $"変数 '{name}' は存在しません。");
			}

			return _VariableDirtyList[index];
		}

		public bool IsVariableDirty(int index)
		{
			Nox.Util.Assert((uint)index < (uint)_VariableDirtyList.Length, "Variable index is out of range. Index={0}", index);
			return _VariableDirtyList[index];
		}

		internal void SetSyncedValue(int index, object? value)
		{
			Nox.Util.Assert((uint)index < (uint)_VariableList.Length, "Variable index is out of range. Index={0}", index);
			_VariableList[index] = value;
			_VariableDirtyList[index] = false;
			SetMatchingPropertyValueFromVariable(RuntimeRecordDecl.VariableList[index].Name, value, isDirty: false);
		}

		public object? GetPropertyValue(string name)
		{
			var index = RuntimeRecordDecl.PropertyList.FindIndex(x => x.Name == name);
			if (index <= -1)
			{
				Nox.Util.Assert(false, $"プロパティ '{name}' は存在しません。");
			}

			return _PropertyValueList[index];
		}

		public void SetPropertyValue(string name, object? value)
		{
			var index = RuntimeRecordDecl.PropertyList.FindIndex(x => x.Name == name);
			if (index <= -1)
			{
				Nox.Util.Assert(false, $"プロパティ '{name}' は存在しません。");
			}

			_PropertyValueList[index] = value;
			_PropertyDirtyList[index] = true;
			SetMatchingVariableValueFromProperty(index, value, isDirty: true);
		}

		public bool IsPropertyDirty(int index)
		{
			Nox.Util.Assert((uint)index < (uint)_PropertyDirtyList.Length, "Property index is out of range. Index={0}", index);
			return _PropertyDirtyList[index];
		}

		internal void SetSyncedPropertyValue(int index, object? value)
		{
			Nox.Util.Assert((uint)index < (uint)_PropertyValueList.Length, "Property index is out of range. Index={0}", index);
			_PropertyValueList[index] = value;
			_PropertyDirtyList[index] = false;
			SetMatchingVariableValueFromProperty(index, value, isDirty: false);
		}

		public void ClearDirtyFlags()
		{
			Array.Clear(_VariableDirtyList);
			Array.Clear(_PropertyDirtyList);
		}

		public virtual void Sync(Core.Net.SyncMode syncMode, Action? callback = null)
		{
			Core.Net.RuntimeRemoteClient remoteClient =
				RemoteClient ?? Core.StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession().RemoteClient;
			Sync(remoteClient, syncMode, callback);

		}

		public void Sync(Core.Net.RuntimeRemoteClient remoteClient, Core.Net.SyncMode syncMode, Action? callback = null)
		{
			if (RemoteInstanceId == 0)
			{
				remoteClient.RegisterRemoteObject(this);
			}

			Span<byte> propertyBuffer = stackalloc byte[2048];
			ReadOnlySpan<byte> propertyBytes = Core.RuntimeRemote.Util.GetPropertiesBytes(propertyBuffer, this);
			Nox.Util.Assert(propertyBytes.Length <= propertyBuffer.Length, "Property buffer is too large.");

			remoteClient.SendQuery(new Core.RuntimeRemote.SyncQuery
			{
				RemoteInstanceId = RemoteInstanceId,
				Fqn = RuntimeFqn,
				PropertyByteBuffer = propertyBytes.ToArray(),
			}, response =>
			{
				Core.RuntimeRemote.SyncResponse syncResponse = Nox.Util.Cast<Core.RuntimeRemote.SyncResponse>(response);
				if (syncResponse.Applied)
				{
					ClearDirtyFlags();
				}
				callback?.Invoke();
			});
		}

		internal void SetRemoteClient(Core.Net.RuntimeRemoteClient remoteClient)
		{
			if (RemoteClient != null && ReferenceEquals(RemoteClient, remoteClient) == false)
			{
				Nox.Util.Assert(false, "RuntimeObject is already registered to another RuntimeRemoteClient.");
			}

			RemoteClient = remoteClient;
		}

    /// <summary>
    /// 型のRuntimeRecordDeclを取得する
    /// </summary>
    public static RuntimeRecordDecl GetRuntimeRecrodDecl<T>() where T : RuntimeObject
    {
        return RuntimeRecordDeclHolder<T>.Value;
    }
    #endregion

    #region 非公開メソッド
		private static object CreateRuntimeValue(RuntimeTypeKind kind)
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

		private static object CreateRuntimeValue(RuntimeTypeInfo typeInfo)
		{
			RuntimeTypeInfo normalizedType = NormalizeRuntimeValueType(typeInfo);
			if (normalizedType.TypeKind == RuntimeTypeKind.Enum && normalizedType.UnderlyingTypeInfo != RuntimeTypeInfo.Invalid)
			{
				return CreateRuntimeValue(normalizedType.UnderlyingTypeInfo.TypeKind);
			}

			switch (normalizedType.TypeKind)
			{
				case RuntimeTypeKind.Class:
				case RuntimeTypeKind.Struct:
				case RuntimeTypeKind.Union:
					return normalizedType.FullName switch
					{
						"nox::Vec3" => default(Vector3),
						"nox::detail::Vector3D<float>" => default(Vector3),
						"nox::Vec3d" => default(Nox.Math.Double3),
						"nox::detail::Vector3D<double>" => default(Nox.Math.Double3),
						"nox::Position" => default(Nox.Position),
						"nox::Quat" => new Nox.Math.Float4 { w = 1.0f },
						"nox::detail::Quaternion<float>" => new Nox.Math.Float4 { w = 1.0f },
						_ => null!,
					};

				default:
					return CreateRuntimeValue(normalizedType.TypeKind);
			}
		}

		private static object CreateRuntimeVariable(RuntimeVariableDecl variableDecl)
		{
			return CreateRuntimeValue(variableDecl.TypeInfo);
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

		private static RuntimeTypeInfo NormalizeRuntimeValueType(RuntimeTypeInfo typeInfo)
		{
			RuntimeTypeInfo current = typeInfo;
			while (current.TypeKind is RuntimeTypeKind.LValueReference or RuntimeTypeKind.RValueReference or RuntimeTypeKind.Pointer)
			{
				if (current.PointeeTypeInfo == RuntimeTypeInfo.Invalid)
				{
					break;
				}

				current = current.PointeeTypeInfo;
			}

			return current;
		}

		private void SetMatchingPropertyValueFromVariable(string variableName, object? value, bool isDirty)
		{
			ReadOnlySpan<RuntimePropertyDecl> propertyList = RuntimeRecordDecl.PropertyList;
			for (int i = 0; i < propertyList.Length; ++i)
			{
				RuntimePropertyDecl propertyDecl = propertyList[i];
				if (propertyDecl.VariableDecl?.Name != variableName)
				{
					continue;
				}

				_PropertyValueList[i] = value;
				_PropertyDirtyList[i] = isDirty;
			}
		}

		private void SetMatchingVariableValueFromProperty(int propertyIndex, object? value, bool isDirty)
		{
			RuntimePropertyDecl propertyDecl = RuntimeRecordDecl.PropertyList[propertyIndex];
			RuntimeVariableDecl? variableDecl = propertyDecl.VariableDecl;
			if (variableDecl == null)
			{
				return;
			}

			int variableIndex = RuntimeRecordDecl.VariableList.FindIndex(x => ReferenceEquals(x, variableDecl) || x.Name == variableDecl.Name);
			if (variableIndex < 0)
			{
				return;
			}

			_VariableList[variableIndex] = value;
			_VariableDirtyList[variableIndex] = isDirty;
		}

		private static string GetRuntimeFqn(System.Type type)
		{
			Core.Attributes.RuntimeWrapperAttribute? attr = type.GetCustomAttribute<Core.Attributes.RuntimeWrapperAttribute>();
			Nox.Util.Assert(attr != null, $"RuntimeWrapperAttribute is required. type={type.FullName}");
			return attr?.RuntimeFQN ?? string.Empty;
		}
    #endregion
}

	public sealed class DynamicRuntimeObject : RuntimeObject
	{
		internal DynamicRuntimeObject(RuntimeRecordDecl runtimeRecordDecl)
			: base(runtimeRecordDecl)
		{
		}
	}
