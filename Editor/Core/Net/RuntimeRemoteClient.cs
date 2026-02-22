using Microsoft.VisualStudio.TextManager.Interop;
using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Net
{
	public enum AutoSyncType : byte
	{
		OneWay,
		OneWaySource,
		TwoWay,
	}

	

	/// <summary>
	/// runtimeプロセスとのIPCクライアント
	/// </summary>
	public class RuntimeRemoteClient : Core.Net.Client, Nox.ISingleton<RuntimeRemoteClient>, IDisposable
	{
		#region 内部型定義
		private enum RemoteInstanceKind : byte
		{
			Runtime,
			Tool,
			_Max
		}
		#endregion

		#region 非公開フィールド
		private uint _QueryIdCounter = 0;

		private readonly Queue<Core.RuntimeRemote.Query> _QueryQueue = new();
		private readonly Dictionary<uint, Action<Core.RuntimeRemote.Response>> _ResponseDict = new();

		private readonly Nox.LockObject _LockWriter = new ();
		private readonly Nox.LockObject _LockQueryList = new ();
		private readonly Nox.ActionHandler _EventRuntimeConnected = new();

		private readonly RemoteWriterStream _WriterStream;
		private readonly RemoteReaderStream _ReaderStream;

		private System.Threading.Thread? _SendThread;
		private System.Threading.Thread? _RecvThread;
		//		private System.Threading.Thread? _DeserializeThread;

		private readonly Dictionary<long, Core.RuntimeObject>[] _RemoteInstanceDictList = Nox.Util.Invoke(() => {
			Dictionary<long, Core.RuntimeObject>[] result = new Dictionary<long, RuntimeObject>[(byte)RemoteInstanceKind._Max];

			for (RemoteInstanceKind i = default; i < RemoteInstanceKind._Max; ++i)
			{
				result[(byte)i] = new Dictionary<long, RuntimeObject>();
			}

			return result;
		});
		#endregion

		#region 公開プロパティ
		public static RuntimeRemoteClient Instance => Nox.ISingleton<RuntimeRemoteClient>.Instance;
		public static bool HasInstance => Nox.ISingleton<RuntimeRemoteClient>.HasInstance;
		#endregion

		#region 公開メソッド
		public static void CreateInstance()
		{
			Nox.ISingleton<RuntimeRemoteClient>.CreateInstance();
		}
		public static void DeleteInstance()
		{
			Nox.ISingleton<RuntimeRemoteClient>.DeleteInstance();
		}

		public RuntimeRemoteClient()
		{
			_WriterStream = new RemoteWriterStream(this);
			_ReaderStream = new RemoteReaderStream(this);
		}

		void IDisposable.Dispose()
		{
			_EventRuntimeConnected.Dispose();
			_WriterStream.Dispose();
			_ReaderStream.Dispose();
		}

		public Nox.DelegateHandle RegisterRuntimeConnectedEvent(Action ev)
		{
			return _EventRuntimeConnected.Add(ev);
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

		
		protected override void OnReceive()
		{
			base.OnReceive();

			Span<byte> recvBuffer = stackalloc byte[8192];
			Receive(recvBuffer);
		}

		public void SendQuery<T>(T query, Action<Core.RuntimeRemote.Response>? response) where T : Core.RuntimeRemote.Query
		{
			uint queryId = System.Threading.Interlocked.Increment(ref _QueryIdCounter);

			lock (_LockQueryList)
			{
				_QueryQueue.Enqueue(query);

				if (response != null)
				{
					_ResponseDict[queryId] = response;
				}
			}
		}
		#endregion

		#region 非公開メソッド
		protected override void OnConneced()
		{
			_SendThread = new System.Threading.Thread(SendProcess);
			_SendThread.Name = "送信スレッド";
			_SendThread.Start();

			_RecvThread = new System.Threading.Thread(RecvProcess);
			_RecvThread.Name =　"受信スレッド";
			_RecvThread.Start();

			_EventRuntimeConnected?.Invoke();
		}

		private void SendProcess()
		{
			Span<char> fqnBuffer = stackalloc char[256];
			Span<byte> fqnBytes = stackalloc byte[512]; // 十分大きいバッファを確保
			
			lock (_LockQueryList)
			{
				foreach (var query in _QueryQueue)
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

		private void RecvProcess()
		{
		}
		#endregion
	}
}
