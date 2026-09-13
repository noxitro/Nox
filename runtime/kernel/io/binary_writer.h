//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	binary_writer.h
///	@brief	binary_writer
#pragma once
#include	"../advanced_type.h"

namespace nox::io
{
	class StreamWriter;

	class BinaryWriter
	{
	public:
		inline explicit BinaryWriter(nox::io::StreamWriter& stream) :
			stream_(stream) {
		}

		void WriteByte(const std::span<const std::byte> buffer)const;
		
		void WriteLength(nox::uint64 length)const;

		void Write(std::u8string_view value)const
		{
			WriteLength(value.length());
			WriteByte({ reinterpret_cast<const std::byte*>(value.data()), value.size() });
		}

		template<typename T> requires(std::is_trivially_copyable_v<T>)
		inline void Write(T value)const
		{
			WriteByte({ reinterpret_cast<const std::byte*>(&value), sizeof(value) });
		}

	private:
		nox::io::StreamWriter& stream_;
	};
}