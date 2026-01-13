using NoxUI;

namespace Core.UI.ViewModels
{
    public enum MessageBoxType
    {
        Error,
        Info,
        Warning,
    }

    public class MessageBoxViewModel : ViewModelBase
    {
        public string Title { get; }
        public string Message { get; }
        public MessageBoxType Type { get; }

        private ViewModelCommand? _okCommand;
        public ViewModelCommand OkCommand => _okCommand ??= new ViewModelCommand(OnOk);

        public event System.Action? RequestClose;

        public MessageBoxViewModel(string message, string? title = null, MessageBoxType type = MessageBoxType.Info)
        {
            Message = message;
            Type = type;
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

        private void OnOk()
        {
            RequestClose?.Invoke();
        }
    }
}
