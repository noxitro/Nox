using Microsoft.VisualStudio.TextManager.Interop;
using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Net
{
	public class RuntimeIpcClient : Core.Net.Client, Nox.ISingleton<RuntimeIpcClient>, IDisposable
	{
		#region 内部クラス定義
		private readonly struct ReqData
		{

		}

		#endregion

		#region 非公開フィールド
		private readonly List<Core.RuntimeRemote.Query> _QueryList = new();
		private readonly Nox.LockObject _LockQueryList = new Nox.LockObject();
		#endregion

		#region 公開プロパティ
		public static RuntimeIpcClient Instance => Nox.ISingleton<RuntimeIpcClient>.Instance;
		public static bool HasInstance => Nox.ISingleton<RuntimeIpcClient>.HasInstance;
		#endregion

		#region 公開メソッド
		public static void CreateInstance()
		{
			Nox.ISingleton<RuntimeIpcClient>.CreateInstance();
		}
		public static void DeleteInstance()
		{
			Nox.ISingleton<RuntimeIpcClient>.DeleteInstance();
		}

		public RuntimeIpcClient()
		{
			_Writer = new RemoteWriter();
		}

		void IDisposable.Dispose()
		{
		}

		private static int ReplaceDotWithDoubleColon(ReadOnlySpan<char> src, Span<char> dst)
		{
			int di = 0;
			for (int si = 0; si < src.Length; ++si)
			{
				if (src[si] == '.')
				{
					if (di + 2 > dst.Length)
						break; // バッファオーバーラン防止
					dst[di++] = ':';
					dst[di++] = ':';
				}
				else
				{
					if (di + 1 > dst.Length)
						break;
					dst[di++] = src[si];
				}
			}
			return di; // 変換後の長さ
		}

		public void UpdateQuery()
		{
			Span<char> fqnBuffer = stackalloc char[256];
			Span<byte> fqnBytes = stackalloc byte[512]; // 十分大きいバッファを確保

			lock (_LockQueryList)
			{
				foreach(var query in _QueryList)
				{
					//	fqnの送信
					{
						ReadOnlySpan<char> fullName = query.GetType().FullName;
						//	.を::に変換
						int fqnLength = ReplaceDotWithDoubleColon(fullName, fqnBuffer);
						int length = System.Text.Encoding.UTF8.GetBytes(fqnBuffer.Slice(0, fqnLength), fqnBytes);

						//	サイズの送信
						Send(BitConverter.GetBytes(length));
						Send(fqnBytes.Slice(0, length));
					}

					//	データの送信
					{

					}
				}
			}
		}
		
		protected override void OnReceive()
		{
			base.OnReceive();

			Span<byte> recvBuffer = stackalloc byte[8192];
			Receive(recvBuffer);


		}

		public void SendQuery<T>(T query) where T : Core.RuntimeRemote.Query
		{
			lock(_LockQueryList)
			{
				_QueryList.Add(query);
			}
		}
		#endregion

		#region 非公開フィールド
		private readonly RemoteWriter _Writer;
		#endregion
	}
}
