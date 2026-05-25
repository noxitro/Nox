using Nox;
using Nox.Extensions;
using System;
using System.Collections.Generic;
using System.Reflection;

namespace Core
{
	public class Runtime : EngineSystem, System.IDisposable
	{
		public static readonly SystemPhaseTerminate<Runtime> TerminatePhase = new(nameof(IDisposable.Dispose), static engineSystem => ((System.IDisposable)engineSystem).Dispose());

		#region 非公開フィールド

        /// <summary>
        /// RuntimeObjectのコンストラクタ辞書
        /// key: RuntimeFQNのハッシュ値(StringComparison.Ordinal)
		/// value: RuntimeObjectのインスタンスを生成するFunc
        /// </summary>
        private readonly IReadOnlyDictionary<int, Func<Core.RuntimeObject>> _RuntimeObjectActivatorDict;

        /// <summary>
        /// RuntimeTypeDeclの辞書
        /// key:RuntimeWrappwerのType
        /// value:Core.RuntimeTypeDecl
        /// </summary>
        private readonly IReadOnlyDictionary<System.Type, Core.RuntimeTypeDecl> _RuntimeRecordDeclDict;
		#endregion

		#region 公開プロパティ
        public PlatformType Platform { get; set; } = PlatformType.X64;
		public ConfigurationType ConfigurationType { get; set; } = ConfigurationType.Debug;
		public RuntimeTypeDB TypeDB { get; set; } = new RuntimeTypeDB();

		public System.Diagnostics.Process? Process => MainRuntimeSession.Process;
		//public Core.RuntimeWrapper.SceneView? MainSceneView => MainRuntimeSession.MainSceneView;
		public event EventHandler? ProcessChanged
		{
			add => MainRuntimeSession.ProcessChanged += value;
			remove => MainRuntimeSession.ProcessChanged -= value;
		}
		public event EventHandler? MainSceneViewChanged
		{
			add => MainRuntimeSession.MainSceneViewChanged += value;
			remove => MainRuntimeSession.MainSceneViewChanged -= value;
		}
		private static RuntimeSession MainRuntimeSession => StudioManager.Instance.Workspace.RuntimeSessions.GetOrCreateMainSession();
		#endregion

		#region 公開メソッド
		public Runtime()
		{
			BuildTypeDB();

			Dictionary<int, Func<Core.RuntimeObject>> activatorDict = new();
			_RuntimeObjectActivatorDict = activatorDict;

			Dictionary<System.Type, Core.RuntimeTypeDecl> runtimeTypeDeclDict = new();
			_RuntimeRecordDeclDict = runtimeTypeDeclDict;

			//	RuntimeWrapper型にDTIを設定する
			System.Type runtimeObjectType = typeof(RuntimeObject);

			foreach (System.Type type in Core.TypeDB.AllTypeList)
			{
				if (runtimeObjectType.IsAssignableFrom(type) == false)
				{
					continue;
				}

				if (runtimeObjectType == type)
				{
					continue;
				}

				Core.Attributes.RuntimeWrapperAttribute? attr = type.GetCustomAttribute<Core.Attributes.RuntimeWrapperAttribute>();
				if (attr == null)
				{
					continue;
				}

				ReadOnlySpan<char> runtimeFQN = attr.RuntimeFQN;
				var runtimeType = TypeDB.FindType(runtimeFQN);
				if (runtimeType == null)
				{
					Nox.LogTrace.ErrorLine<Core.LogId.Runtime>($"RuntimeWrapperAttributeで指定された型がTypeDBに存在しません:{runtimeFQN}");
					continue;
				}

				RuntimeRecordDecl runtimeRecordDecl = (RuntimeRecordDecl)runtimeType.Decl;
				var holderType = typeof(Core.RuntimeObject.RuntimeRecordDeclHolder<>).MakeGenericType(type);
				holderType.GetField("Value")!.SetValue(null, runtimeRecordDecl);

				// ⭐ string 生成なしでハッシュ計算
				int hash = runtimeFQN.GetHashCode(StringComparison.Ordinal);
				Nox.Util.Assert(activatorDict.ContainsKey(hash) == false, $"RuntimeFQNのハッシュ値が重複しています:{runtimeFQN}");

				var ctor = type.GetConstructor(Type.EmptyTypes);
				if (ctor == null)
				{
					Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>($"型 {type.FullName} にデフォルトコンストラクタがありません。");
					continue;
				}

				var factory = System.Linq.Expressions.Expression
											.Lambda<Func<Core.RuntimeObject>>(
												System.Linq.Expressions.Expression.New(ctor))
											.Compile();

				activatorDict.Add(hash, factory);
			}
		}

		public override PhaseRegister[] GetPhaseRegisterList()
		{
			return
			[
				PhaseRegister.Create(TerminatePhase, this),
			];
		}

		void IDisposable.Dispose()
		{
			StudioManager.Instance.Workspace.RuntimeSessions.FindMainSession()?.Dispose();
		}

		public bool Reboot()
		{
			RuntimeSession runtimeSession = MainRuntimeSession;
			runtimeSession.Platform = Platform;
			runtimeSession.ConfigurationType = ConfigurationType;
			return runtimeSession.Reboot();
		}

		public void StartTcpConnection()
		{
			MainRuntimeSession.StartTcpConnection();
		}

		public void PlayRuntime()
		{

		}

		/// <summary>
		/// RuntimeFQNからRuntimeObjectを生成します
		/// </summary>
		/// <param name="runtimeFQN"></param>
		/// <returns></returns>
		public Core.RuntimeObject? CreateRuntimeObject(ReadOnlySpan<char> runtimeFQN)
		{
			if (_RuntimeObjectActivatorDict.TryGetValue(runtimeFQN.GetHashCode(StringComparison.Ordinal), out var factory) == false)
			{
				RuntimeRecordDecl? runtimeRecordDecl = TypeDB.FindRecordDecl(runtimeFQN);
				return runtimeRecordDecl == null ? null : new DynamicRuntimeObject(runtimeRecordDecl);
			}

			return factory();
		}
		#endregion

		#region 非公開メソッド

		private void BuildTypeDB()
		{
			TypeDB = new RuntimeTypeDB();
			TypeDB.Build(Platform, ConfigurationType);
		}

		#endregion
	}
}
