using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Core.RuntimeRemote
{
	public static class Util
	{
		private static readonly Dictionary<Type, int> _BitCopySizeCache = [];
		private static readonly object _BitCopySizeLock = new();

		/// <summary>
		/// editorへ公開されている変数かどうか
		/// </summary>
		/// <param name="variableInfo"></param>
		/// <returns></returns>
		public static bool IsRemoteVariable(Core.RuntimeVariableDecl variableInfo)
		{
			foreach (var attr in variableInfo.AttributeList)
			{
				switch (attr)
				{
					case System.Runtime.Serialization.DataMemberAttribute:
					//	ignore datamemberは非シリアライズだが、editorへ公開はする
					case System.Runtime.Serialization.IgnoreDataMemberAttribute:
						return true;
				}
			}
			return false;
		}

		public static ReadOnlySpan<byte> GetPropertiesBytes(Span<byte> buffer, Core.RuntimeObject runtimeObject)
		{
			Nox.StackMemoryWriter writer = new(buffer);
			ReadOnlySpan<Core.RuntimePropertyDecl> propertyInfoList = runtimeObject.RuntimeRecordDecl.PropertyList;
			ReadOnlySpan<object> propertyList = runtimeObject.PropertyValueList;
			for (int i = 0, length = propertyInfoList.Length; i < length; ++i)
			{
				object propertyValue = propertyList[i];
				if (propertyValue == null)
				{
					continue;
				}

				if (TryWriteBitCopyValue(ref writer, propertyValue))
				{
					continue;
				}

				TypeCode typeCode = Type.GetTypeCode(propertyValue.GetType());

				switch (typeCode)
				{
					case TypeCode.Boolean: writer.Write((bool)propertyValue); break;
					case TypeCode.SByte: writer.Write((sbyte)propertyValue); break;
					case TypeCode.Byte: writer.Write((byte)propertyValue); break;
					case TypeCode.Int16: writer.Write((short)propertyValue); break;
					case TypeCode.UInt16: writer.Write((ushort)propertyValue); break;
					case TypeCode.Int32: writer.Write((int)propertyValue); break;
					case TypeCode.UInt32: writer.Write((uint)propertyValue); break;
					case TypeCode.Int64: writer.Write((long)propertyValue); break;
					case TypeCode.UInt64: writer.Write((ulong)propertyValue); break;
					case TypeCode.Single: writer.Write((float)propertyValue); break;
					case TypeCode.Double: writer.Write((double)propertyValue); break;
					case TypeCode.String: writer.Write((string)propertyValue); break;
					default:
						//	未対応の型
						Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>($"Unsupported property type. type:{propertyValue.GetType().FullName}");
						break;
				}
			}

			return writer.WrittenSpan;
		}

		public static bool SetPropertiesFromBytes(ReadOnlySpan<byte> buffer, Core.RuntimeObject runtimeObject)
		{
			Nox.StackMemoryStreamReader reader = new(buffer);
			ReadOnlySpan<Core.RuntimePropertyDecl> propertyInfoList = runtimeObject.RuntimeRecordDecl.PropertyList;
			Span<object> propertyList = runtimeObject.RefPropertyValueList;
			bool hasChanges = false;
			for (int i = 0, length = propertyInfoList.Length; i < length; ++i)
			{
				object propertyValue = propertyList[i];
				if (propertyValue == null)
				{
					continue;
				}

				object? syncedValue = null;
				bool valueRead = true;
				if (TryReadBitCopyValue(ref reader, propertyValue.GetType(), out object? structValue))
				{
					syncedValue = structValue;
				}
				else
				{
					TypeCode typeCode = Type.GetTypeCode(propertyValue.GetType());
					switch (typeCode)
					{
						case TypeCode.Boolean:
							syncedValue = reader.ReadBool();
							break;
						case TypeCode.SByte:
							syncedValue = reader.ReadSByte();
							break;
						case TypeCode.Byte:
							syncedValue = reader.ReadByte();
							break;
						case TypeCode.Int16:
							syncedValue = reader.ReadInt16();
							break;
						case TypeCode.UInt16:
							syncedValue = reader.ReadUInt16();
							break;
						case TypeCode.Int32:
							syncedValue = reader.ReadInt32();
							break;
						case TypeCode.UInt32:
							syncedValue = reader.ReadUInt32();
							break;
						case TypeCode.Int64:
							syncedValue = reader.ReadInt64();
							break;
						case TypeCode.UInt64:
							syncedValue = reader.ReadUInt64();
							break;
						case TypeCode.Single:
							syncedValue = reader.ReadFloat();
							break;
						case TypeCode.Double:
							syncedValue = reader.ReadDouble();
							break;
						case TypeCode.String:
							syncedValue = reader.ReadString();
							break;
						default:
							valueRead = false;
							//	未対応の型
							Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>($"Unsupported property type. type:{propertyValue.GetType().FullName}");
							break;
					}
				}

				if (valueRead && runtimeObject.IsPropertyDirty(i) == false)
				{
					if (Equals(propertyValue, syncedValue) == false)
					{
						runtimeObject.SetSyncedPropertyValue(i, syncedValue);
						hasChanges = true;
					}
				}
			}

			return hasChanges;
		}

		private static bool TryWriteBitCopyValue(ref Nox.StackMemoryWriter writer, object variable)
		{
			Type type = variable.GetType();
			if (TryGetBitCopySize(type, out int size) == false)
			{
				return false;
			}

			GCHandle handle = default;
			try
			{
				handle = GCHandle.Alloc(variable, GCHandleType.Pinned);
				unsafe
				{
					writer.WriteBytes(new ReadOnlySpan<byte>(handle.AddrOfPinnedObject().ToPointer(), size));
				}
				return true;
			}
			finally
			{
				if (handle.IsAllocated)
				{
					handle.Free();
				}
			}
		}

		private static bool TryReadBitCopyValue(ref Nox.StackMemoryStreamReader reader, Type type, out object? value)
		{
			if (TryGetBitCopySize(type, out int size) == false)
			{
				value = null;
				return false;
			}

			object boxed = Activator.CreateInstance(type)!;
			GCHandle handle = default;
			try
			{
				handle = GCHandle.Alloc(boxed, GCHandleType.Pinned);
				ReadOnlySpan<byte> bytes = reader.ReadBytes(size);
				unsafe
				{
					bytes.CopyTo(new Span<byte>(handle.AddrOfPinnedObject().ToPointer(), size));
				}
				value = boxed;
				return true;
			}
			finally
			{
				if (handle.IsAllocated)
				{
					handle.Free();
				}
			}
		}

		private static bool TryGetBitCopySize(Type type, out int size)
		{
			lock (_BitCopySizeLock)
			{
				if (_BitCopySizeCache.TryGetValue(type, out size))
				{
					return size > 0;
				}

				if (type.IsValueType == false || type.IsPrimitive || type == typeof(decimal))
				{
					_BitCopySizeCache[type] = 0;
					size = 0;
					return false;
				}

				object sample = Activator.CreateInstance(type)!;
				try
				{
					using GCHandleScope pinned = new(sample);
					size = Marshal.SizeOf(type);
					_BitCopySizeCache[type] = size;
					return true;
				}
				catch (ArgumentException)
				{
					_BitCopySizeCache[type] = 0;
					size = 0;
					return false;
				}
			}
		}

		private readonly ref struct GCHandleScope
		{
			private readonly GCHandle _Handle;

			public GCHandleScope(object value)
			{
				_Handle = GCHandle.Alloc(value, GCHandleType.Pinned);
			}

			public void Dispose()
			{
				if (_Handle.IsAllocated)
				{
					_Handle.Free();
				}
			}
		}
	}
}
