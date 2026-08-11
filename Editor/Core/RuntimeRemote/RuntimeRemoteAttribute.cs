using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote.Attributes;

	/// <summary>
	/// runtimeパス下に出力します
	/// </summary>
	[System.AttributeUsage(AttributeTargets.Class)]
	public class RuntimeRemoteCodeAttribute : System.Attribute
	{
		public RuntimeRemoteCodeAttribute(string path, string namespaceStr = "nox::dev::editor_remote", bool execute = true, bool enabledSend = true, bool enabledRecv = true, string comment = "")
		{
			Path = path;
			EnabledExecute = execute;
			EnabledSend = enabledSend;
			EnabledRecv = enabledRecv;
			Comment = comment;
			NamespaceStr = namespaceStr;
    }

		/// <summary>
		/// 出力先パス
		/// ソリューションディレクトリからの相対パス
		/// </summary>
		public string Path { get; init; }

    public string NamespaceStr { get; init; }

		/// <summary>
		/// execute関数をcpp側で実装するか
		/// Queryの場合のみ有効
		/// </summary>
		public bool EnabledExecute { get; init; }

		/// <summary>
		/// c++側でのコメント
		/// </summary>
		public string Comment { get; init; }

		public bool EnabledSend { get; init; }
		public bool EnabledRecv { get; init; }
	}

	internal sealed class CoreRuntimeRemoteCodeAttribute : RuntimeRemoteCodeAttribute
	{
		public CoreRuntimeRemoteCodeAttribute(string filename, bool execute = true, bool enabledSend = true, bool enabledRecv = true, string comment = "")
			: base($"core/dev/remote/remote_{filename}", "nox::dev::editor_remote", execute, enabledSend, enabledRecv, comment)
		{
		}
	}

	[System.AttributeUsage(AttributeTargets.Property)]
	public sealed class NativeRuntimeFQNAttribute : System.Attribute
	{
		public NativeRuntimeFQNAttribute(string fqn)
		{
			FQN = fqn;
		}

		public string FQN { get; init; }
	}

	/// <summary>
	/// c#側のstringをcpp側で固定長配列(fixed_string)として扱うための属性
	/// </summary>
	[System.AttributeUsage(System.AttributeTargets.Property)]
	public sealed class FixedStringAttribute : System.Attribute
	{
		public FixedStringAttribute(uint length)
		{
			Length = length;
		}

		public uint Length { get; init; }
	}

	/// <summary>
	/// std::u8string_viewとして扱う
	/// </summary>
	[System.AttributeUsage(System.AttributeTargets.Property)]
	public sealed class StringViewAttribute : System.Attribute
	{
	}

/// <summary>
/// c++側で固定長配列として扱うための属性
/// </summary>
public sealed class FixedArrayAttribute : System.Attribute
	{
		public FixedArrayAttribute(uint length)
		{
			Length = length;
		}
		public uint Length { get; init; }
}
