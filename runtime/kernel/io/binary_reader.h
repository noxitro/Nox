//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	binary_reader.h
///	@brief	binary_reader
#pragma once
#include	"../advanced_type.h"

namespace nox::io
{
	class StreamReaderBase;

	class BinaryReader
	{
	public:
		inline constexpr explicit BinaryReader(nox::io::StreamReaderBase& stream)noexcept :
			stream_(stream) {
		}

		void ReadBytes(std::span<std::byte> dest)const;

		nox::uint64 ReadLength()const;

		template<typename T> requires(std::is_trivially_copyable_v<T>)
		inline void Read(T& out)const
		{
			ReadBytes({ reinterpret_cast<std::byte*>(&out), sizeof(T) });
		}

	private:
		nox::io::StreamReaderBase& stream_;
	};
}