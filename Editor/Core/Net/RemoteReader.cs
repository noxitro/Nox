using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Net
{
	public class RemoteReader
	{
		#region 公開メソッド
		public RemoteReader(RuntimeIpcClient client)
		{
			_Client = client;
		}

		public void Read<T>(out T value) where T : struct, System.Numerics.INumber<T>
		{
			value = default;
		}

		public T Read<T>() where T : struct, System.Numerics.INumber<T>
		{
			return default;
		}
		#endregion

		#region 非公開フィールド
		private readonly RuntimeIpcClient _Client;
		#endregion
	}
}
