// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Nox;

	public ref struct FixedString
	{
		#region 非公開フィールド
		private Span<char> _Buffer;
		private int _Length;
		#endregion

		#region 公開プロパティ
		public int Length => _Length;
		public int Capacity => _Buffer.Length;
		#endregion

		#region 公開メソッド
		public FixedString(Span<char> buffer)
		{
			_Buffer = buffer;
			_Length = 0;
		}

		public ReadOnlySpan<char> AsSpan()
		{
			return _Buffer.Slice(0, _Length);
		}

		public override string ToString()
		{
			// 必要なときだけ string を生成（GC を気にする箇所では呼ばない）
			return _Buffer.Slice(0, _Length).ToString();
		}

		public void Clear()
		{
			_Length = 0;
		}

		public bool TryAppend(char c)
		{
			if (_Length >= _Buffer.Length)
			{
				return false;
			}

			_Buffer[_Length++] = c;
			return true;
		}

		public bool TryAppend(ReadOnlySpan<char> value)
		{
			if (value.Length > _Buffer.Length - _Length)
			{
				return false;
			}

			value.CopyTo(_Buffer.Slice(_Length));
			_Length += value.Length;
			return true;
		}

		/// <summary>
		/// 簡易フォーマット: {0} のみサポート。
		/// {{ と }} はそれぞれ { と } として扱う。
		/// </summary>
		public bool TryAppendFormat(ReadOnlySpan<char> format, ReadOnlySpan<char> arg0)
		{
			return TryAppendFormatCore(format, arg0, default, default, default, argCount: 1);
		}

		/// <summary>
		/// 簡易フォーマット: {0}, {1} をサポート。
		/// {{ と }} はそれぞれ { と } として扱う。
		/// </summary>
		public bool TryAppendFormat(ReadOnlySpan<char> format, ReadOnlySpan<char> arg0, ReadOnlySpan<char> arg1)
		{
			return TryAppendFormatCore(format, arg0, arg1, default, default, argCount: 2);
		}

		#endregion

		#region 非公開メソッド
		private bool TryAppendFormatCore(
		ReadOnlySpan<char> format,
		ReadOnlySpan<char> arg0,
		ReadOnlySpan<char> arg1,
		ReadOnlySpan<char> arg2,
		ReadOnlySpan<char> arg3,
		int argCount)
		{
			for (int i = 0; i < format.Length; i++)
			{
				char c = format[i];

				if (c == '{')
				{
					// エスケープ: "{{" -> "{"
					if (i + 1 < format.Length && format[i + 1] == '{')
					{
						if (!TryAppend('{'))
						{
							return false;
						}

						i++;
						continue;
					}

					// プレースホルダ: "{0}"〜"{3}"
					if (i + 2 < format.Length && char.IsDigit(format[i + 1]) && format[i + 2] == '}')
					{
						int index = format[i + 1] - '0';

						if ((uint)index >= 4u || index >= argCount)
						{
							// 未対応 or 未設定のインデックス
							return false;
						}

						ReadOnlySpan<char> arg = index switch
						{
							0 => arg0,
							1 => arg1,
							2 => arg2,
							3 => arg3,
							_ => default
						};

						if (!TryAppend(arg))
						{
							return false;
						}

						i += 2;
						continue;
					}

					// それ以外はそのまま
					if (!TryAppend(c))
					{
						return false;
					}
				}
				else if (c == '}')
				{
					// エスケープ: "}}" -> "}"
					if (i + 1 < format.Length && format[i + 1] == '}')
					{
						if (!TryAppend('}'))
						{
							return false;
						}

						i++;
						continue;
					}

					// 単独の '}' はそのまま
					if (!TryAppend(c))
					{
						return false;
					}
				}
				else
				{
					if (!TryAppend(c))
					{
						return false;
					}
				}
			}

			return true;
		}
		#endregion
	}
