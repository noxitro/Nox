using System;
using System.Collections.Generic;

namespace Core;

	public abstract class EngineModule
	{
  private static EngineModule? _Instance;
		private readonly List<EngineSystem> _EngineSystemList = new();
		private bool _Finalized;

   public virtual EngineSystem[] CreateEngineSystems()
		{
			return Array.Empty<EngineSystem>();
		}

		public static EngineModule Instance => _Instance ?? throw new InvalidOperationException("EngineModule is not initialized.");

		public static void CreateInstance()
		{
			_ = InstanceOrCreate;
		}

		public static void DeleteInstance()
		{
			if (_Instance == null)
			{
				return;
			}

     _Instance.InvokeFinalize();
			_Instance = null;
		}

		private static EngineModule InstanceOrCreate
		{
			get
			{
				if (_Instance == null)
				{
					_Instance = new CoreModule();
				}

				return _Instance;
			}
		}

		protected EngineModule()
		{
		}

		public void InvokeStart()
		{
			StudioManager.CreateInstance();
      _Finalized = false;
			_EngineSystemList.Clear();
			_EngineSystemList.AddRange(CreateEngineSystems());

			foreach (EngineSystem engineSystem in _EngineSystemList)
			{
				StudioManager.Instance.RegisterEngineSystem(engineSystem);
			}

			StudioManager.Instance.InvokeInit();
			StudioManager.Instance.InvokeStart();
		}

		public void InvokeFinalize()
		{
      if (_Finalized)
			{
				return;
			}

       _Finalized = true;
			StudioManager.DeleteInstance();
		}
	}


	file class CoreModule : EngineModule
	{
 public CoreModule()
		{
   }

		public override EngineSystem[] CreateEngineSystems()
		{
			return
			[
				new Core.Net.SocketScheduler(),
				new Core.Runtime(),
       ];
		}
	}
