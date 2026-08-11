using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Data;

namespace Core.UI.ViewModels;

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

		public string Channel
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

	public class LogViewModel : Core.UI.ViewModels.DocumentViewModel, IDisposable
	{
		private const int MaxItemCount = 10000;
		private const int MaxDrainCountPerTick = 1000;

		#region 非公開フィールド
		private readonly Core.LogService _LogService;
		private readonly System.Windows.Threading.DispatcherTimer _DrainTimer;
		private NoxUI.ViewModelCommand? _ClearCommand;
		private string _SearchText = string.Empty;
		private string _SelectedLevelFilter = AllLevelsFilter;
		private string _SelectedChannelFilter = AllChannelsFilter;
		private bool _Disposed;
		#endregion

		private const string AllLevelsFilter = "All Levels";
		private const string AllChannelsFilter = "All Channels";

		#region 公開プロパティ
		public ObservableCollection<LogItemViewModel> Items { get; } = new();
		public ObservableCollection<string> LevelFilters { get; } = new() { AllLevelsFilter };
		public ObservableCollection<string> ChannelFilters { get; } = new() { AllChannelsFilter };
		public ICollectionView FilteredItems { get; }
		public string StatusText => IsFilterActive == false
			? $"{Items.Count} trace item(s)"
			: $"{FilteredItems.Cast<object>().Count()} / {Items.Count} trace item(s)";
		public NoxUI.ViewModelCommand ClearCommand => _ClearCommand ??= new(Clear);

		public string SearchText
		{
			get => _SearchText;
			set
			{
				if (SetProperty(ref _SearchText, value))
				{
					FilteredItems.Refresh();
					RaisePropertyChanged(nameof(StatusText));
				}
			}
		}

		public string SelectedLevelFilter
		{
			get => _SelectedLevelFilter;
			set
			{
				value = string.IsNullOrEmpty(value) ? AllLevelsFilter : value;
				if (SetProperty(ref _SelectedLevelFilter, value))
				{
					RefreshFilter();
				}
			}
		}

		public string SelectedChannelFilter
		{
			get => _SelectedChannelFilter;
			set
			{
				value = string.IsNullOrEmpty(value) ? AllChannelsFilter : value;
				if (SetProperty(ref _SelectedChannelFilter, value))
				{
					RefreshFilter();
				}
			}
		}

		private bool IsFilterActive =>
			string.IsNullOrWhiteSpace(SearchText) == false ||
			SelectedLevelFilter != AllLevelsFilter ||
			SelectedChannelFilter != AllChannelsFilter;
		#endregion

		public LogViewModel(Core.LogService logService)
		{
			_LogService = logService;
			FilteredItems = CollectionViewSource.GetDefaultView(Items);
			FilteredItems.Filter = FilterLogItem;
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

		private bool FilterLogItem(object item)
		{
			if (item is not LogItemViewModel logItem)
			{
				return false;
			}

			if (MatchFilters(logItem) == false)
			{
				return false;
			}

			if (string.IsNullOrWhiteSpace(SearchText))
			{
				return true;
			}

			return Contains(logItem.Time, SearchText)
				|| Contains(logItem.Level, SearchText)
				|| Contains(logItem.Source, SearchText)
				|| Contains(logItem.Channel, SearchText)
				|| Contains(logItem.Message, SearchText);
		}

		private bool MatchFilters(LogItemViewModel logItem)
		{
			if (SelectedLevelFilter != AllLevelsFilter && logItem.Level != SelectedLevelFilter)
			{
				return false;
			}

			if (SelectedChannelFilter != AllChannelsFilter && logItem.Channel != SelectedChannelFilter)
			{
				return false;
			}

			return true;
		}

		private static bool Contains(string value, string searchText)
		{
			return value.Contains(searchText, System.StringComparison.OrdinalIgnoreCase);
		}

		private void Add(Core.LogService.Data data)
		{
			AddFilterOption(LevelFilters, data.LogLevel);
			AddFilterOption(ChannelFilters, data.Channel);

			Items.Add(new LogItemViewModel
			{
				Time = data.TimeStamp.ToString("HH:mm:ss"),
				Level = data.LogLevel,
				Source = data.Source,
				Channel = data.Channel,
				Message = data.Message,
			});
		}

		private void RefreshFilter()
		{
			FilteredItems.Refresh();
			RaisePropertyChanged(nameof(StatusText));
		}

		private static void AddFilterOption(ObservableCollection<string> options, string value)
		{
			if (string.IsNullOrWhiteSpace(value) || options.Contains(value))
			{
				return;
			}

			options.Add(value);
		}

		private void Clear()
		{
			_LogService.ClearPending();
			Items.Clear();
			ResetFilters();
			FilteredItems.Refresh();
			RaisePropertyChanged(nameof(StatusText));
		}

		private void ResetFilters()
		{
			LevelFilters.Clear();
			ChannelFilters.Clear();
			LevelFilters.Add(AllLevelsFilter);
			ChannelFilters.Add(AllChannelsFilter);
			_SelectedLevelFilter = string.Empty;
			_SelectedChannelFilter = string.Empty;
			SelectedLevelFilter = AllLevelsFilter;
			SelectedChannelFilter = AllChannelsFilter;
		}

		private void TrimOverflowItems()
		{
			int removeCount = Items.Count - MaxItemCount;
			for (int i = 0; i < removeCount; ++i)
			{
				Items.RemoveAt(0);
			}
		}

		public override void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			_DrainTimer.Stop();
			_Disposed = true;
		}
		#endregion
	}
