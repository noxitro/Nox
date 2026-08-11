using System;

namespace Core;

public static partial class Util
{
    public static string ToRuntimeFQN(string fqn)
    {
        string r = fqn.Replace(".", "::");
        //	クラス内定義の場合は+が付くので、それも::に変換
        return r.Replace("+", "::");
    }

    /// <summary>
    /// RuntimeFQNに変換する
    /// 
    /// 例: Core.RuntimeObject -> Core::RuntimeObject
    /// </summary>
    /// <param name="buffer"></param>
    /// <param name="fqn"></param>
    /// <returns></returns>
    /// <exception cref="ArgumentException"></exception>
    public static ReadOnlySpan<char> ToRuntimeFQN(Span<char> buffer, ReadOnlySpan<char> fqn)
    {
        ReadOnlySpan<char> fqnSpan = fqn;
        int bufferIndex = 0;
        for (int i = 0, length = fqnSpan.Length; i < length; ++i)
        {
            char c = fqnSpan[i];
            if (c == '.' || c == '+')
            {
                buffer[bufferIndex++] = ':';
                buffer[bufferIndex++] = ':';
            }
            else
            {
                buffer[bufferIndex++] = c;
            }
        }
        if (bufferIndex > buffer.Length)
        {
            throw new ArgumentException($"Buffer is too small for runtime FQN. required={bufferIndex}, actual={buffer.Length}");
        }
        return buffer.Slice(0, bufferIndex);
    }

    /// <summary>
    /// runtime(c++)のFQNをeditor(c#)のFQNに変換する
    /// </summary>
    public static ReadOnlySpan<char> ToEditorFQN(Span<char> buffer, ReadOnlySpan<char> fqn)
    {
        ReadOnlySpan<char> fqnSpan = fqn;
        int bufferIndex = 0;
        for (int i = 0, length = fqnSpan.Length; i < length; ++i)
        {
            char c = fqnSpan[i];
            if (c == ':' && i + 1 < length && fqnSpan[i + 1] == ':')
            {
                buffer[bufferIndex++] = '.';
                ++i;
            }
            else
            {
                buffer[bufferIndex++] = c;
            }
        }
        if (bufferIndex > buffer.Length)
        {
            throw new ArgumentException($"Buffer is too small for editor FQN. required={bufferIndex}, actual={buffer.Length}");
        }
        return buffer.Slice(0, bufferIndex);
    }

    public static string GetNamespaceFromRuntimeFQN(ReadOnlySpan<char> fqn)
    {
        int lastSepIndex = fqn.LastIndexOf("::");
        return lastSepIndex >= 0 ? fqn.Slice(0, lastSepIndex).ToString() : string.Empty;
    }

    public static string GetNameFromRuntimeFQN(ReadOnlySpan<char> fqn)
    {
        int lastSepIndex = fqn.LastIndexOf("::");
        return lastSepIndex >= 0 ? fqn.Slice(lastSepIndex + 2).ToString() : fqn.ToString();
    }
}
