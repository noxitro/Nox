using NoxUI;

namespace Core.UI.ViewModels
{
    public enum MessageBoxType : byte
    {
        Error,
        Info,
        Warning,
    }

    public enum MessageBoxStyle : byte
    {
        Yes,
        YesNo,
        YesNoCancel
    }

	public enum MessageBoxResult : byte
    {
        Yes,
        No,
		Cancel,
        Close = Cancel,
	}

	public class MessageBoxViewModel : ViewModelBase
    {
        public string Title { get; }
        public string Message { get; }
        public MessageBoxType Type { get; }
        public MessageBoxStyle Style { get; }

        public ViewModelCommand YesCommand => field ??= new ViewModelCommand(OnYes);
        public ViewModelCommand NoCommand => field ??= new ViewModelCommand(OnNo);
        public ViewModelCommand CancelCommand => field ??= new ViewModelCommand(OnCancel);

        public event System.Action<MessageBoxResult>? RequestClose;

        public MessageBoxViewModel() { }

		public MessageBoxViewModel(string message, string? title = null, MessageBoxType type = MessageBoxType.Info, MessageBoxStyle style = MessageBoxStyle.Yes)
        {
            Message = message;
            Type = type;
            Style = style;
            Title = title ?? GetDefaultTitle(type);
        }

        private static string GetDefaultTitle(MessageBoxType type)
        {
            return type switch
            {
                MessageBoxType.Error => "Error",
                MessageBoxType.Warning => "Warning",
                _ => "Information",
            };
        }

        private void OnYes() => RequestClose?.Invoke(MessageBoxResult.Yes);

        private void OnNo() => RequestClose?.Invoke(MessageBoxResult.No);

        private void OnCancel() => RequestClose?.Invoke(MessageBoxResult.Cancel);
    }
}
