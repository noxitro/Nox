using Microsoft.VisualStudio.TextManager.Interop;
using Nox.Utility;
using System;
using System.Collections.Generic;
using System.Reflection;
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
		#endregion

		#region 非公開フィールド
		private const ushort FQN_BYTES_MAX = 512; // FQNの最大バイト数（UTF-8エンコード後）

		private uint _QueryIdCounter = 0;
		private long _RemoteInstanceIdCounter = 0;

		private readonly Queue<Core.RuntimeRemote.Query> _QueryQueue = new();
		private readonly Dictionary<uint, Action<Core.RuntimeRemote.Response>> _ResponseDict = new();

		private readonly System.Threading.Lock _LockWriter = new ();
		private readonly System.Threading.Lock _LockQueryList = new ();
		private readonly Nox.ActionHandler _EventRuntimeConnected = new();

		private readonly RemoteWriterStream _WriterStream;
		private readonly RemoteReaderStream _ReaderStream;

		private System.Threading.Thread? _SendThread;
		private System.Threading.Thread? _RecvThread;
		private System.Threading.Thread? _DeserializeThread;
		private readonly System.Threading.ManualResetEventSlim _SendSignal = new();
		private readonly System.Threading.ManualResetEventSlim _DeserializeSignal = new();

		//	インスタンスIDが正の数ならエディタオブジェクト、負の数ならランタイムオブジェクトとする
		private readonly Dictionary<long, Core.RuntimeObject> _RuntimeObjectDict = new();
		private readonly Dictionary<long, Core.RuntimeObject> _EditorObjectDict = new();

		private Nox.Utility.RWParallelExecuteChecker _RemoteObjectDictParallelExecuteChecker = new ();
		private readonly System.Threading.ReaderWriterLockSlim _RemoteDictRWLock = new();

		private readonly IReadOnlyDictionary<int, Func<Core.RuntimeRemote.Entity>> _RemoteEntityTypeDict;
		#endregion

		#region 非公開プロパティ
		private bool IsDisconnected => this._Socket == null || Entity.CheckDisconnected(this._Socket);
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

			{
				Span<char> runtimeFQNBuffer = stackalloc char[512];

				//	リフレクションでRuntimeRemote.Query/Responseのサブクラスを列挙してIDと型の対応表を作る
				Dictionary<int, Func<Core.RuntimeRemote.Entity>> dict = new();
				_RemoteEntityTypeDict = dict;

				foreach (var type in TypeDB.AllTypeList)
				{
					if (type.IsAbstract)
					{
						continue;
					}

					if (type.IsSubclassOf(typeof(Core.RuntimeRemote.Entity)) == false)
					{
						continue;
					}
					var attr = type.GetCustomAttribute<Core.RuntimeRemote.Attributes.RuntimeRemoteCodeAttribute>();
					if (attr == null)
					{
						continue;
					}

					// ⭐ 受信側と同じFQN形式でキーを作る
					// "nox::dev::editor_remote::SendLog" → "nox.dev.editor_remote.SendLog"
					bool success = MemoryExtensions.TryWrite(runtimeFQNBuffer, $"{attr.NamespaceStr}::{type.Name}", out int written);
					Nox.Util.Assert(success, "FQN バッファが不足しています");

					// ⭐ string 生成なしでハッシュ計算
					int hash = string.GetHashCode(runtimeFQNBuffer.Slice(0, written), StringComparison.Ordinal);

					if (dict.ContainsKey(hash))
					{
						Nox.Util.Assert(false, "FQN ハッシュの衝突が発生しています");
						continue;
					}

					// ⭐ Expression.New でコンストラクタ呼び出しをコンパイル → Activator 不要
					var ctor = type.GetConstructor(Type.EmptyTypes);
					if (ctor == null)
					{
						Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>($"型 {type.FullName} にデフォルトコンストラクタがありません。");
						continue;
					}

					var factory = System.Linq.Expressions.Expression
											.Lambda<Func<Core.RuntimeRemote.Entity>>(
												System.Linq.Expressions.Expression.New(ctor))
											.Compile();

					dict.Add(hash, factory);
				}
			}
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

		//	Span<byte> recvBuffer = stackalloc byte[8192];
		//	ReceiveAll(recvBuffer);
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

			_SendSignal.Set();
		}

		public void RegisterRemoteObject(Core.RuntimeObject obj)
		{
			long instanceId = System.Threading.Interlocked.Increment(ref _RemoteInstanceIdCounter);
			RegisterRemoteObject(obj, instanceId);
		}

		public void RegisterRemoteObject(Core.RuntimeObject obj, long instanceId)
		{
			using var _ = _RemoteObjectDictParallelExecuteChecker.EnterWriteScope();

			Dictionary<long, Core.RuntimeObject> dict = instanceId >= 0 ? _EditorObjectDict : _RuntimeObjectDict;
			if (dict.TryAdd(instanceId, obj) == false)
			{
				Nox.Util.Assert(false, $"インスタンスID {instanceId} は既に登録されています");
			}

			obj.RemoteInstanceId = instanceId;
		}

		public Core.RuntimeObject? FindRemoteInstance(long instanceId)
		{
			using var _ = _RemoteObjectDictParallelExecuteChecker.EnterReadScope();

			IReadOnlyDictionary<long, Core.RuntimeObject> dict = instanceId >= 0 ? _EditorObjectDict : _RuntimeObjectDict;
			
			if (dict.TryGetValue(instanceId, out var obj))
			{
				return obj;
			}
			return null;
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

			_DeserializeThread = new System.Threading.Thread(DeserializeProcess);
			_DeserializeThread.Name = "Deserializeスレッド";
			_DeserializeThread.Start();

			_EventRuntimeConnected?.Invoke();
		}

		protected override void OnDisconnected()
		{
			base.OnDisconnected();

			if (_SendThread != null)
			{
				_SendSignal.Set();
				_SendThread.Join();
				_SendThread = null;
			}

			if (_RecvThread != null)
			{
				_RecvThread.Join();
				_RecvThread = null;
			}

			if (_DeserializeThread != null)
			{
				_DeserializeSignal.Set();
				_DeserializeThread.Join();
				_DeserializeThread = null;
			}
		}

		/// <summary>
		/// バッファ送信スレッド
		/// </summary>
		private void SendProcess()
		{
			Span<char> fqnBuffer = stackalloc char[256];
			Span<byte> fqnBytes = stackalloc byte[FQN_BYTES_MAX]; // 十分大きいバッファを確保

			while (IsDisconnected == false)
			{
				_SendSignal.Wait();
				_SendSignal.Reset();

				lock (_LockQueryList)
				{
					bool failed = false;
					using (var binaryWriter = new System.IO.BinaryWriter(_WriterStream))
					{
						foreach (var query in _QueryQueue)
						{
							
							//	データの送信
							try
							{
								query.Serialize(binaryWriter);
								binaryWriter.Flush();
							}
							catch (System.InvalidOperationException)
							{
								//	切断中の送信エラーは無視
								Nox.LogTrace.InfoLine<Core.LogId.RuntimeRemote>("切断中の送信エラーを無視します");
								failed = true;
							}

							if (failed)
							{
								break;
							}
						}
					}

					if (failed)
					{
						_QueryQueue.Clear();
					}
				}
			}
		}

		/// <summary>
		/// 受信スレッド
		/// </summary>
		private void RecvProcess()
		{
			Span<byte> buffer = stackalloc byte[2048]; // 受信バッファ

			while (IsDisconnected == false)
			{
				if (IsRecvReadable(16) == false)
				{
					continue;
				}

				// 到着データがある限り吸い出す
				bool signaled = false;
				while (true)
				{
					if (_Socket==null)
					{
						break;
					}
				//	int availableSize = _Socket.Available;
					int received = Receive(buffer);
					if (received > 0)
					{
						_ReaderStream.AddBuffer(buffer.Slice(0, received));

						// 1パケット分揃った時だけ通知
						if (!signaled && _ReaderStream.CanReadBody())
						{
							_DeserializeSignal.Set();
							signaled = true;
						}
					}
					else if (received < 0)
					{
						Nox.LogTrace.InfoLine<Core.LogId.RuntimeRemote>("切断されました");
						return;
					}
					else
					{
						continue;
					}

					if (IsRecvReadable(0) == false)
					{
						break;
					}
				}
			}
		}

		/// <summary>
		/// 受信したバッファをデシリアライズしてQueryのResponseを処理するスレッド
		/// </summary>
		private void DeserializeProcess()
		{
			Span<char> fqnBuffer = stackalloc char[512];
			while (IsDisconnected == false)
			{
				_DeserializeSignal.Wait();
				_DeserializeSignal.Reset();

				using var binaryReader = new System.IO.BinaryReader(_ReaderStream, System.Text.Encoding.UTF8, leaveOpen: true);

				// 1パケット分揃っている間は連続処理

				while (_ReaderStream.CanReadBody())
				{
					// パケット全長ヘッダ（LEB128）を消費
					binaryReader.Read7BitEncodedInt64();

					// FQN
					ReadOnlySpan<char> runtimeFQN = binaryReader.ReadString();
					// ✅ コンストラクタ側と同じ方法でハッシュ計算
					int fqnHash = string.GetHashCode(runtimeFQN, StringComparison.Ordinal);
					if (_RemoteEntityTypeDict.TryGetValue(fqnHash, out Func<Core.RuntimeRemote.Entity>? factory) == false)
					{
						Nox.Util.Assert(false, $"{runtimeFQN}がdictに存在しません");
					}

					Core.RuntimeRemote.Entity entity = factory();

					if (entity is Core.RuntimeRemote.Query query)
					{
						Nox.Util.Assert(query != null, $"型 {runtimeFQN} のインスタンスを作成できませんでした");
						if (query == null) continue;
						query.Deserialize(binaryReader);
						query.Execute();
					}
					else if (entity is Core.RuntimeRemote.Response response)
					{
						Nox.Util.Assert(response != null, $"型 {runtimeFQN} のインスタンスを作成できませんでした");
						if (response == null) continue;
						response.Deserialize(binaryReader);

						if (_ResponseDict.TryGetValue(response.Id, out var callback))
						{
							callback.Invoke(response);
						}
						else
						{
							Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>(
								$"クエリID {response.Id} に対応するコールバックが見つかりませんでした");
						}
					}
					else
					{
						Nox.Util.Assert(false, $"型 {runtimeFQN} は Query でも Response でもありません");
					}
				}
			}
		}
		#endregion
	}
}
