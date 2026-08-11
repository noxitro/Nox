namespace Core.RuntimeRemote.LogId;

internal readonly struct RuntimeRemote : Nox.LogId.ILogId<RuntimeRemote>
{
    public RuntimeRemote() { }
    string Nox.LogId.ILogId<RuntimeRemote>.Tag { get; } = "Runtime";
}
