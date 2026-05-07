using System.Collections.ObjectModel;

namespace Core.UI.ViewModels
{
	public class LogItemViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public string Time
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;

		public string Level
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;

		public string Source
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;

		public string Message
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;
		#endregion
	}

	public class LogViewModel : Core.UI.ViewModels.DocumentViewModel
	{
		private const int MaxItemCount = 10000;
		private const int MaxDrainCountPerTick = 1000;

		#region 非公開フィールド
		private readonly Core.LogService _LogService;
		private readonly System.Windows.Threading.DispatcherTimer _DrainTimer;
		private NoxUI.ViewModelCommand? _ClearCommand;
		#endregion

		#region 公開プロパティ
		public ObservableCollection<LogItemViewModel> Items { get; } = new();
		public string StatusText => $"{Items.Count} trace item(s)";
		public NoxUI.ViewModelCommand ClearCommand => _ClearCommand ??= new(Clear);
		#endregion

		public LogViewModel(Core.LogService logService)
		{
			_LogService = logService;
			_DrainTimer = new System.Windows.Threading.DispatcherTimer
			{
				Interval = System.TimeSpan.FromMilliseconds(50),
			};
			_DrainTimer.Tick += (_, _) => DrainPendingLogs();
			_DrainTimer.Start();
		}

		#region 非公開メソッド
		private void DrainPendingLogs()
		{
			int addedCount = 0;

			while (addedCount < MaxDrainCountPerTick && _LogService.TryDequeue(out Core.LogService.Data data))
			{
				Add(data);
				++addedCount;
			}

			if (addedCount == 0)
			{
				return;
			}

			TrimOverflowItems();
			RaisePropertyChanged(nameof(StatusText));
		}

		private void Add(Core.LogService.Data data)
		{
			Items.Add(new LogItemViewModel
			{
				Time = data.TimeStamp.ToString("HH:mm:ss"),
				Level = data.LogLevel,
				Source = data.Source,
				Message = data.Message,
			});
		}

		private void Clear()
		{
			_LogService.ClearPending();
			Items.Clear();
			RaisePropertyChanged(nameof(StatusText));
		}

		private void TrimOverflowItems()
		{
			int removeCount = Items.Count - MaxItemCount;
			for (int i = 0; i < removeCount; ++i)
			{
				Items.RemoveAt(0);
			}
		}
		#endregion
	}
}
