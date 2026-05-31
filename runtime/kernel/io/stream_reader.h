//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	stream_reader.h
///	@brief	stream_reader
#pragma once
#include	"stream.h"
#include	"../os/file.h"

namespace nox::io
{
	class StreamReaderBase : public nox::io::Stream
	{
	public:
		inline constexpr StreamReaderBase()noexcept {}
		inline constexpr virtual ~StreamReaderBase()noexcept override {}

		virtual void Read(std::span<std::byte> dest) = 0;
	};

	class FileStreamReader final : public nox::io::StreamReaderBase
	{
	public:
		inline explicit FileStreamReader(std::u8string_view path);

		void Read(std::span<std::byte> dest)override;
	private:
		nox::io::u8ifstream ifs_;
	};

	class SpanStreamReader final: public StreamReaderBase
	{
	public:
		inline constexpr explicit SpanStreamReader(std::span<std::byte> buffer)noexcept :
			buffer_(buffer),
			position_(0)
		{
		}
		void Read(std::span<std::byte> dest)override;
	private:
		std::span<std::byte> buffer_;
		nox::uint32 position_;
	};

	/// @brief ヒープアロケーションのないファイルストリームリーダー
	class FileSpanStreamReader final : public StreamReaderBase
	{
	public:
		inline explicit FileSpanStreamReader(std::u8string_view path);

		void Read(std::span<std::byte> dest)override;
	private:
		nox::os::File file_;
	};
}