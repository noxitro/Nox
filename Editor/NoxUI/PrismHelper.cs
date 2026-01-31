using System;
using System.Collections.Generic;
using System.Text;

namespace NoxUI
{
	public static class PrismHelper
	{
		public static T? ResolveDataContext<T>() where T : class
		{
			var app = System.Windows.Application.Current as Prism.Unity.PrismApplication;
			if (app == null)
			{
				return null;
			}

			IContainerProvider container = app.Container;

			return container.Resolve<T>();
		}
	}
}
