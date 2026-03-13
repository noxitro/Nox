using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote
{
	public static class Util
	{
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
			ReadOnlySpan<Core.RuntimeVariableDecl> variableInfoList = runtimeObject.RuntimeRecordDecl.VariableList;
			ReadOnlySpan<object> variableList = runtimeObject.VariableList;
			for (int i = 0, length = variableInfoList.Length; i < length; ++i)
			{
				if (IsRemoteVariable(variableInfoList[i]) == false)
				{
					continue;
				}

				RuntimeVariableDecl variableINfo = variableInfoList[i];
				object variable = variableList[i];

				TypeCode typeCode = Type.GetTypeCode(variable.GetType());

				switch (typeCode)
				{
					case TypeCode.Boolean: writer.Write((bool)variable); break;
					case TypeCode.SByte: writer.Write((sbyte)variable); break;
					case TypeCode.Byte: writer.Write((byte)variable); break;
					case TypeCode.Int16: writer.Write((short)variable); break;
					case TypeCode.UInt16: writer.Write((ushort)variable); break;
					case TypeCode.Int32: writer.Write((int)variable); break;
					case TypeCode.UInt32: writer.Write((uint)variable); break;
					case TypeCode.Int64: writer.Write((long)variable); break;
					case TypeCode.UInt64: writer.Write((ulong)variable); break;
					case TypeCode.Single: writer.Write((float)variable); break;
					case TypeCode.Double: writer.Write((double)variable); break;
					case TypeCode.String: writer.Write((string)variable); break;
					default:
						//	未対応の型
						Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>($"Unsupported variable type. type:{variable.GetType().FullName}");
						break;
				}
			}

			return writer.WrittenSpan;
		}

		public static void SetPropertiesFromBytes(ReadOnlySpan<byte> buffer, Core.RuntimeObject runtimeObject)
		{
			Nox.StackMemoryStreamReader reader = new(buffer);
			ReadOnlySpan<Core.RuntimeVariableDecl> variableInfoList = runtimeObject.RuntimeRecordDecl.VariableList;
			Span<object> variableList = runtimeObject.RefVariableList;
			for (int i = 0, length = variableInfoList.Length; i < length; ++i)
			{
				if (IsRemoteVariable(variableInfoList[i]) == false)
				{
					continue;
				}

				object variable = variableList[i];
				TypeCode typeCode = Type.GetTypeCode(variable.GetType());
				switch (typeCode)
				{
					case TypeCode.Boolean:
						variableList[i] = reader.ReadBool();
						break;
					case TypeCode.SByte: 
						variableList[i] = reader.ReadSByte();
						break;
					case TypeCode.Byte: 
						variableList[i] = reader.ReadByte(); 
						break;
					case TypeCode.Int16: 
						variableList[i] = reader.ReadInt16(); 
						break;
					case TypeCode.UInt16: 
						variableList[i] = reader.ReadUInt16(); 
						break;
					case TypeCode.Int32: 
						variableList[i] = reader.ReadInt32(); 
						break;
					case TypeCode.UInt32: 
						variableList[i] = reader.ReadUInt32(); 
						break;
					case TypeCode.Int64: 
						variableList[i] = reader.ReadInt64(); 
						break;
					case TypeCode.UInt64: 
						variableList[i] = reader.ReadUInt64(); 
						break;
					case TypeCode.Single: 
						variableList[i] = reader.ReadFloat(); 
						break;
					case TypeCode.Double: 
						variableList[i] = reader.ReadDouble(); 
						break;
					case TypeCode.String: 
						variableList[i] = reader.ReadString(); 
						break;
					default:
						//	未対応の型
						Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>($"Unsupported variable type. type:{variable.GetType().FullName}");
						break;
				}
			}
		}

	}
}
