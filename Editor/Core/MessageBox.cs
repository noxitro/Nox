using System;
using System.Collections.Generic;
using System.Text;

namespace Core
{
	public enum MessageLevel : byte
	{
		Info,
		Warning,
		Error,
	}

	public interface IMessageService
	{
		void ShowMessage(string message, string? title = null, MessageLevel level = MessageLevel.Info);
	}

	public static class MessageServiceProvider
	{
		private static IMessageService? _Current = null;
		public static IMessageService Current
		{
			get
			{
				if (_Current == null)
				{
					throw new InvalidOperationException("MessageService is not registered.");
				}
				return _Current;
			}
		}
		public static void Register(IMessageService service)
		{
			_Current = service;
		}
	}
}