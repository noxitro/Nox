using Core.Net;
using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote
{
	public abstract class Entity
	{
		public uint Id { get; private set; }

		#region 公開メソッド
		public void Serialize(uint id, BinaryWriter writer)
		{
			//	type fqn
			System.Type type = GetType();
			ReadOnlySpan<char> fullName = type.FullName;
			writer.Write(fullName);

			//	id
			writer.Write(Id);

			ReadOnlySpan<System.Reflection.PropertyInfo> propList = type.GetProperties();	
			foreach(var prop in propList)
			{
				WriteProperty(writer, prop);
			}
		}

		public void Deserialize(BinaryReader reader)
		{
			Id = reader.ReadUInt32();


		}
		#endregion

		#region 非公開メソッド
		private void WriteProperty(BinaryWriter writer, System.Reflection.PropertyInfo prop)
		{
			System.Type propType = prop.PropertyType;
			prop.GetValue(this);
		}
		#endregion
	}
}
