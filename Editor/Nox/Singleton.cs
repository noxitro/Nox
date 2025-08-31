using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Nox
{
	public interface ISingleton<T>
#if DEBUG
		: IDisposable
#endif
		where T : class, new()
	{
		#region 公開
		public static T Instance
		{
			get
			{
				System.Diagnostics.Debug.Assert(_Instance != null, "Singleton instance not initialized. Call Initialize() first.");
				return _Instance;
			}
		}

		public static bool HasInstance => _Instance != null;

		public static void CreateInstance()
		{
			System.Diagnostics.Debug.Assert(_Instance == null, "Singleton instance already initialized.");
			_Instance = new T();
		}

		public static void DeleteInstance()
		{
			System.Diagnostics.Debug.Assert(_Instance != null, "Singleton instance not initialized.");
			_Instance = null;
		}
		#endregion

		#region 非公開
		private static T? _Instance = null;

#if DEBUG
		void IDisposable.Dispose()
		{
			System.Diagnostics.Debug.Assert(_Instance != null, "Singleton instance not initialized.");
			_Instance = null;
		}
#endif
		#endregion
	}

	public class Runtime : ISingleton<Runtime>
	{
		public void test()
		{
		}
	}


}
