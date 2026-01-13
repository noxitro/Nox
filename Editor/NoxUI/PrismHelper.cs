using System;
using System.Collections.Generic;
using System.Text;

namespace NoxUI
{
	public static class PrismHelper
	{
		public static T ResolveDataContext<T>()
		{
			var app = (Prism.Unity.PrismApplication)System.Windows.Application.Current;
			IContainerProvider container = app.Container;

			return container.Resolve<T>();
		}
	}
}
