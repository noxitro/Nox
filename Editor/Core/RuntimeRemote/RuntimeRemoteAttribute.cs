using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote.Attr
{
	/// <summary>
	/// runtimeパス下に出力します
	/// </summary>
	[System.AttributeUsage(AttributeTargets.Class)]
	public sealed class RuntimeRemoteCodeAttribute : System.Attribute
	{
		public RuntimeRemoteCodeAttribute(string path, bool execute = true, string comment = "")
		{
			Path = path;
			EnabledExecute = execute;
			Comment = comment;
		}

		/// <summary>
		/// 出力先パス
		/// ソリューションディレクトリからの相対パス
		/// </summary>
		public string Path { get; init; }

		/// <summary>
		/// execute関数をcpp側で実装するか
		/// Queryの場合のみ有効
		/// </summary>
		public bool EnabledExecute { get; init; }

		/// <summary>
		/// c++側でのコメント
		/// </summary>
		public string Comment { get; init; }
	}

	[System.AttributeUsage(AttributeTargets.Property)]
	public sealed class RuntimeRemoteCodeNativeFQNAttribute : System.Attribute
	{
		public RuntimeRemoteCodeNativeFQNAttribute(string fqn)
		{
			FQN = fqn;
		}

		public string FQN { get; init; }
	}

	/// <summary>
	/// c#側のarrayをcpp側で固定長配列として扱うための属性
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
}
