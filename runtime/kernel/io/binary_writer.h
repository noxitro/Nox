//	Copyright (C) 2026 NOX ENGINE All rights reserved.

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

		void WriteByte(const std::span<const nox::uint8> buffer)const;
		
		void WriteLength(nox::uint64 length)const;

		void Write(std::u8string_view value)const
		{
			WriteLength(value.length());
			WriteByte({ reinterpret_cast<const nox::uint8*const>(value.data()), value.size() });
		}

		template<std::integral T>
		inline void Write(T value)const
		{
			WriteByte({ reinterpret_cast<const nox::uint8*>(value), sizeof(T)});
		}

		template<std::floating_point T>
		inline void Write(T value)const
		{
			WriteByte({ reinterpret_cast<const nox::uint8*>(value), sizeof(T) });
		}

	private:
		nox::io::StreamWriter& stream_;
	};
}