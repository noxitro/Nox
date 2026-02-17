//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	binary_reader.h
///	@brief	binary_reader
#pragma once
#include	"../advanced_type.h"

namespace nox::io
{
	class StreamReader;

	class BinaryReader
	{
	public:
		inline constexpr explicit BinaryReader(nox::io::StreamReader& stream)noexcept :
			stream_(stream) {
		}

		void ReadBytes(std::span<nox::uint8> dest)const;

		nox::uint64 ReadLength()const;

		template<std::integral T>
		inline void Read(T& out)const
		{
			ReadBytes({ reinterpret_cast<nox::uint8*>(&out), sizeof(T) });
		}

		template<std::floating_point T>
		inline void Read(T& out)const
		{
			ReadBytes({ reinterpret_cast<nox::uint8*>(&out), sizeof(T) });
		}
	private:
		nox::io::StreamReader& stream_;
	};
}