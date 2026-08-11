namespace Nox.LogId;

public interface ILogId<T> where T : struct, ILogId<T>
{
    public string Tag => typeof(T).Name;
}

public readonly struct Unknown : ILogId<Unknown>
{
    public Unknown() { }
    public string Tag { get; } = nameof(Unknown);
}

public readonly struct Kernel : ILogId<Kernel>
{
    public Kernel() { }
    public string Tag { get; } = nameof(Kernel);
}
