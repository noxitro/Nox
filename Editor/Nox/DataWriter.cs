using System;
using System.Collections.Generic;
using System.Text;

namespace Nox
{
	/// <summary>
	/// バイナリデータライター（System.IO.BinaryWriter の上位互換）
	/// ゼロアロケーション・Span ベースの高速シリアライズ
	/// </summary>
	public struct DataWriter : System.IDisposable
	{
		#region 非公開フィールド
		private readonly System.IO.Stream _Stream;
		#endregion

		#region 公開プロパティ
		#endregion

		#region 公開メソッド
		public DataWriter(System.IO.Stream stream)
		{
			_Stream = stream;
		}

		public void Dispose()
		{
			_Stream.Dispose();
		}

		public void Write(ReadOnlySpan<char> value)
		{
			
		}
		#endregion

		#region 非公開メソッド

		#endregion
	}
}
