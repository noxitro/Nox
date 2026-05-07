using System;

namespace Core.UI
{
	public static class ProjectSettingsViewService
	{
		private static Action? _Show;

		public static void Register(Action show)
		{
			_Show = show;
		}

		public static void Unregister(Action show)
		{
			if (_Show == show)
			{
				_Show = null;
			}
		}

		public static void Show()
		{
			_Show?.Invoke();
		}
	}
}
