// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core;

	public static class RuntimeTypeUtil
	{
		public static ReadOnlySpan<char> GetPrimitiveTypeName(TypeCode typeCode)
		{
			switch (typeCode)
			{
				case TypeCode.Boolean: return "bool";
				case TypeCode.SByte: return "nox::int8";
				case TypeCode.Byte: return "nox::uint8";
				case TypeCode.Int16: return "nox::int16";
				case TypeCode.UInt16: return "nox::uint16";
				case TypeCode.Int32: return "nox::int32";
				case TypeCode.UInt32: return "nox::uint32";
				case TypeCode.Int64: return "nox::int64";
				case TypeCode.UInt64: return "nox::uint64";
				case TypeCode.Single: return "float";
				case TypeCode.Double: return "double";
				case TypeCode.Char: return "nox::char8";
			}

			Nox.Util.Assert(false, "not found primitive type:{0}", typeCode.ToString());
			return "";
		}
	}
