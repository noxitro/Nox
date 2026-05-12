using System;
using System.Collections.ObjectModel;

namespace Core.UI.ViewModels
{
	public sealed class InspectorSyncIntervalOption
	{
		public required string DisplayName { get; init; }
		public int Milliseconds { get; init; }
		public bool IsManual => Milliseconds <= 0;
	}

	public sealed class InspectorSyncSettings : NoxUI.ViewModelBase
	{
		public static InspectorSyncSettings Instance { get; } = new();

		public ObservableCollection<InspectorSyncIntervalOption> Options { get; } =
		[
			new() { DisplayName = "Manual", Milliseconds = 0 },
			new() { DisplayName = "50ms", Milliseconds = 50 },
			new() { DisplayName = "500ms", Milliseconds = 500 },
			new() { DisplayName = "1000ms", Milliseconds = 1000 },
		];

		public InspectorSyncIntervalOption SelectedOption
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(IsManual));
					Changed?.Invoke(this, EventArgs.Empty);
				}
			}
		}

		public bool IsManual => SelectedOption.IsManual;

		public event EventHandler? Changed;
		public event EventHandler? ManualSyncRequested;

		private InspectorSyncSettings()
		{
			SelectedOption = Options[2];
		}

		public void RequestManualSync()
		{
			ManualSyncRequested?.Invoke(this, EventArgs.Empty);
		}
	}
}
