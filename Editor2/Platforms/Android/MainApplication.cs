namespace Nox
{
    [global::Android.App.Application]
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    internal class MainApplication : Microsoft.Maui.MauiApplication
    {
        public MainApplication(IntPtr handle, Android.Runtime.JniHandleOwnership ownership)
            : base(handle, ownership)
        {
        }

        protected override MauiApp CreateMauiApp() => MauiProgram.CreateMauiApp();
    }
}
