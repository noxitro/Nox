namespace NoxUI
{
	public abstract class ViewModelBase : System.ComponentModel.INotifyPropertyChanged, IDisposable
	{
		public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;

		protected void SetProperty<T>(ref T field, T value, [System.Runtime.CompilerServices.CallerMemberName] string? propertyName = null)
		{
			if (Equals(field, value))
				return;

			field = value;
			PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
		}

		public void Dispose()
		{
			// 共通の解放処理があればここに
		}
	}
}
