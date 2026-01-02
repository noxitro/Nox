using Core.RuntimeWrapper;
using Nox;
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

namespace Core
{
	public class Runtime : Nox.ISingleton<Runtime>
	{
		#region 非公開フィールド

		private System.Diagnostics.Process? _Process = default;
		#endregion

		#region 公開プロパティ
		public static Runtime Instance => Nox.ISingleton<Runtime>.Instance;
		public PlatformType Platform { get; set; } = PlatformType.X64;
		public ConfigurationType ConfigurationType { get; set; } = ConfigurationType.Debug;
		public RuntimeTypeDB TypeDB { get; set; } = new RuntimeTypeDB();
		#endregion

		#region 公開メソッド
		public static void CreateInstance()
		{
			Nox.ISingleton<Runtime>.CreateInstance();
		}
		public static void DeleteInstance()
		{
			Nox.ISingleton<Runtime>.DeleteInstance();
		}

		public Runtime()
		{
			Initialize();
		}

		public void Reboot()
		{
			Task.Run(() =>
			{
				if (_Process != null)
				{
					_Process.Kill();
					_Process = null;
				}

				string runtimeExePath = $"runtime\\build\\runtime\\{Platform.GetName()}\\{ConfigurationType.GetName()}\\runtime.exe";
				Nox.Util.Assert(System.IO.File.Exists(runtimeExePath) == true, "Runtime.exeが存在しません:{0}");

				string args = "-Studio";
				_Process = System.Diagnostics.Process.Start(runtimeExePath, args);

				//	runtimeへ接続
				Core.Net.RuntimeIpcClient.Instance.Startup(new Net.Client.InitializeContext() { 
					Hostname = "localhost",
					Port = 86
				});
			});
		}
		#endregion

		#region 非公開メソッド
		private void Initialize()
		{
			BuildTypeDB();

			//	RuntimeWrapper型にDTIを設定する
			System.Type runtimeObjectType = typeof(RuntimeObject);
			System.Type runtimeObjectInterfaceType = typeof(IRuntimeObject<>);
			string runtimeObjectInterfaceTypeFullName = runtimeObjectInterfaceType.FullName ?? string.Empty;

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

				Type? interfaceType = type.GetInterface(runtimeObjectInterfaceTypeFullName);
				if (interfaceType == null)
				{
					continue;
				}

				var property = interfaceType.GetProperty("Type");
				if (property == null)
				{
					continue;
				}
				
				Core.Attributes.RuntimeWrapperAttribute? attr = type.GetCustomAttribute<Core.Attributes.RuntimeWrapperAttribute>();
				if (attr == null)
				{
					continue;
				}

				RuntimeTypeInfo? runtimeType = TypeDB.FindType(attr.FQN);
				Nox.Util.Assert(runtimeType != null, "RuntimeWrapperAttributeで指定された型がTypeDBに存在しません:{0}", attr.FQN);

				property.SetValue(null, runtimeType);
			}
		}

		private void BuildTypeDB()
		{
			TypeDB = new RuntimeTypeDB();
			TypeDB.Build(Platform, ConfigurationType);
		}
		#endregion
	}
}
