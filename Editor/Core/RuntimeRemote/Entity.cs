using Core.Net;
using Nox.Extensions;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

namespace Core.RuntimeRemote
{
	public abstract class Entity
	{
		public uint Id { get; private set; }

		#region 公開メソッド
		public void Serialize(BinaryWriter writer)
		{
			//	runtimeの型名へ変換
			Type type = GetType();
			writer.Write(GetRuntimeTypeFQN(type));

			//	id
			writer.Write(Id);

			//	properties
			IReadOnlyList<PropertyInfo> propList = GetRemotePropertyInfoList(type);
			foreach (var prop in propList)
			{
				object? value = prop.GetValue(this);
				Nox.Util.Assert(value != null,
					$"Entity property value must not be null. type={type.FullName}, prop={prop.Name}");
				if (value == null) continue;

				Type propType = prop.PropertyType;

				if (propType.IsPrimitive)
				{
					// Fix: propType を使う（type は Entity 自身の型のため誤り）
					WritePrimitive(writer, Type.GetTypeCode(propType), value);
				}
				else if (propType.IsEnum)
				{
					Type underlyingType = Enum.GetUnderlyingType(propType);
					WritePrimitive(writer, Type.GetTypeCode(underlyingType),
						Convert.ChangeType(value, underlyingType));
				}
				else if (propType == typeof(string))
				{
					writer.Write((string)value);
				}
				else
				{
					Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>(
						$"Unsupported property type. type:{propType.FullName}");
				}
			}
		}

		public void Deserialize(BinaryReader reader)
		{
			Id = reader.ReadUInt32();
		}
		#endregion

		#region 非公開フィールド
		/// <summary>型ごとのプロパティリストキャッシュ（リフレクションコスト削減）</summary>
		private static readonly ConcurrentDictionary<Type, IReadOnlyList<PropertyInfo>> _PropertyInfoCache = new();
		#endregion

		#region 非公開メソッド
		private static string GetRuntimeTypeFQN(Type type)
		{
			var attr = type.GetCustomAttribute<Core.RuntimeRemote.Attributes.RuntimeRemoteCodeAttribute>();
			Nox.Util.Assert(attr != null, $"RuntimeRemoteCodeAttribute is required. type={type.FullName}");

			string str = $"{attr.NamespaceStr}::{type.Name}";
			return str;
		}

		private static void WritePrimitive(BinaryWriter writer, TypeCode typeCode, object value)
		{
			switch (typeCode)
			{
				case TypeCode.Boolean: writer.Write((bool)value);   break;
				case TypeCode.SByte:   writer.Write((sbyte)value);  break;
				case TypeCode.Byte:    writer.Write((byte)value);   break;
				case TypeCode.Int16:   writer.Write((short)value);  break;
				case TypeCode.UInt16:  writer.Write((ushort)value); break;
				case TypeCode.Int32:   writer.Write((int)value);    break;
				case TypeCode.UInt32:  writer.Write((uint)value);   break;
				case TypeCode.Int64:   writer.Write((long)value);   break;
				case TypeCode.UInt64:  writer.Write((ulong)value);  break;
				case TypeCode.Single:  writer.Write((float)value);  break;
				case TypeCode.Double:  writer.Write((double)value); break;
				default:
					Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>(
						$"Unsupported TypeCode: {typeCode}");
					break;
			}
		}

		/// <summary>
		/// シリアライズ対象プロパティ一覧を返す（型ごとにキャッシュ済み）
		/// </summary>
		private static IReadOnlyList<PropertyInfo> GetRemotePropertyInfoList(Type type)
		{
			return _PropertyInfoCache.GetOrAdd(type, static t =>
			{
				// private プロパティも含める（LogQuery.Str 等）
				var propList = t.GetProperties(
					BindingFlags.Public |
					BindingFlags.NonPublic |
					BindingFlags.Instance |
					BindingFlags.DeclaredOnly);

				var result = new List<PropertyInfo>();
				foreach (var prop in propList)
				{
					if (!prop.CanRead || !prop.CanWrite) continue;
					// Id は Entity 基底が書き込み済みのため除外
					if (prop.Name == nameof(Id)) continue;
					result.Add(prop);
				}
				return result.AsReadOnly();
			});
		}
		#endregion
	}
}
