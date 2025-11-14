//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	stream.cpp
///	@brief	stream
#include	"stdafx.h"
#include	"stream.h"

#include	"../assertion.h"
#include	"../unicode_converter.h"
#include	"../algorithm.h"
#if defined(_WIN32)
#	define NOMINMAX
#	include <Windows.h>
#endif

namespace nox::assertion::id
{
	struct StreamError : ErrorId
	{
		inline constexpr std::u32string_view operator()() const noexcept { return U"StreamError"; }
	};
}

namespace nox::io
{
	// =========================
	// MemoryStream
	// =========================
	MemoryStream::MemoryStream(nox::Vector<nox::uint8> buffer, bool writable) noexcept
		: owned_(static_cast<nox::Vector<nox::uint8>&&>(buffer))
		, view_(nullptr)
		, size_(static_cast<nox::uint64>(owned_.size()))
		, pos_(0)
		, owned_size_(static_cast<nox::uint64>(owned_.size()))
		, writable_(writable)
	{
	}

	MemoryStream::MemoryStream(const nox::uint8* data, nox::uint64 size) noexcept
		: owned_()
		, view_(data)
		, size_(size)
		, pos_(0)
		, owned_size_(0)
		, writable_(false)
	{
		NOX_ASSERT(data != nullptr || size == 0, U"MemoryStream: data が nullptr なのに size > 0 です");
	}

	bool MemoryStream::Seek(long long offset, SeekOrigin origin) const noexcept
	{
		long long base = 0;
		switch (origin)
		{
		case SeekOrigin::Begin:   base = 0; break;
		case SeekOrigin::Current: base = static_cast<long long>(pos_); break;
		case SeekOrigin::End:     base = static_cast<long long>(size_); break;
		}
		const long long next = base + offset;
		if (next < 0) { NOX_ASSERT(false, U"MemoryStream::Seek: 負の位置は無効です"); return false; }
		const nox::uint64 p = static_cast<nox::uint64>(next);
		if (p > size_) { NOX_ASSERT(false, U"MemoryStream::Seek: 範囲外の位置です"); return false; }
		pos_ = p;
		return true;
	}

	nox::uint64 MemoryStream::Read(void* dst, nox::uint64 bytes) const
	{
		NOX_ASSERT(dst != nullptr || bytes == 0, U"MemoryStream::Read: dst が nullptr です");
		if (bytes == 0) return 0;
		const nox::uint64 avail = (pos_ <= size_) ? (size_ - pos_) : 0;
		const nox::uint64 n = (bytes <= avail) ? bytes : avail;
		if (n > 0)
		{
			const nox::uint8* src = (view_ != nullptr) ? view_ : (owned_size_ ? owned_.data() : nullptr);
			NOX_ASSERT(src != nullptr, U"MemoryStream::Read: 内部バッファが無効です");
			::memcpy(dst, src + pos_, static_cast<size_t>(n));
			pos_ += n;
		}
		NOX_ASSERT(n == bytes, U"MemoryStream::Read: 予期せぬEOF/読み取り不足");
		return n;
	}

	bool MemoryStream::Write(const void* src, nox::uint64 bytes)
	{
		NOX_ASSERT(CanWrite(), U"MemoryStream::Write: 書き込み不可のストリームです");
		if (!CanWrite()) return false;
		if (bytes == 0) return true;

		const nox::uint64 need = pos_ + bytes;
		if (need > static_cast<nox::uint64>(owned_.size()))
		{
			owned_.resize(static_cast<size_t>(need));
			view_ = nullptr;
			size_ = static_cast<nox::uint64>(owned_.size());
			owned_size_ = size_;
		}
		NOX_ASSERT(src != nullptr, U"MemoryStream::Write: src が nullptr です");
		::memcpy(owned_.data() + pos_, src, static_cast<size_t>(bytes));
		pos_ += bytes;
		return true;
	}

	void MemoryStream::Close() noexcept
	{
		owned_.clear();
		view_ = nullptr;
		size_ = 0;
		pos_ = 0;
		owned_size_ = 0;
		writable_ = true;
	}

	nox::Vector<nox::uint8> MemoryStream::MoveBuffer() noexcept
	{
		nox::Vector<nox::uint8> tmp = static_cast<nox::Vector<nox::uint8>&&>(owned_);
		owned_.clear();
		view_ = nullptr;
		size_ = 0;
		pos_ = 0;
		owned_size_ = 0;
		return tmp;
	}

	// =========================
	// FileStream（std未使用, Win32）
	// =========================
	bool FileStream::Open(std::u32string_view path, FileMode mode, FileAccess access)
	{
		Close();
		mode_ = mode;
		access_ = access;
		pos_ = 0;
		length_ = 0;
		dirty_ = false;

		path_u16_ = nox::unicode::ConvertU16String(path);

#if defined(_WIN32)
		// 読み込み要求ありなら全読み込み（存在しない場合は 0 長として扱う）
		if ((static_cast<nox::uint8>(access_) & nox::util::ToUnderlying(FileAccess::Read)) != 0)
		{
			HANDLE h = ::CreateFileW(reinterpret_cast<LPCWSTR>(path_u16_.data()),
				GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (h != INVALID_HANDLE_VALUE)
			{
				LARGE_INTEGER li{};
				const BOOL okSize = ::GetFileSizeEx(h, &li);
				NOX_ASSERT(okSize, U"FileStream::Open: GetFileSizeEx に失敗しました");
				if (okSize)
				{
					const nox::uint64 sz = static_cast<nox::uint64>(li.QuadPart);
					if (sz > 0)
					{
						buffer_.resize(static_cast<size_t>(sz));
						DWORD readBytes = 0;
						const BOOL ok = ::ReadFile(h, buffer_.data(), static_cast<DWORD>(sz), &readBytes, nullptr);
						NOX_ASSERT(ok && readBytes == sz, U"FileStream::Open: ReadFile に失敗しました");
						length_ = static_cast<nox::uint64>(readBytes);
					}
				}
				::CloseHandle(h);
			}
			else
			{
				NOX_ASSERT(mode_ == FileMode::OpenOrCreate || mode_ == FileMode::Create || mode_ == FileMode::CreateNew,
					U"FileStream::Open: ファイルが存在せず Open モードで失敗しました");
			}
		}
#else
		NOX_ASSERT(false, U"FileStream::Open: 非Windowsは未実装です");
		return false;
#endif
		length_ = static_cast<nox::uint64>(buffer_.size());
		return true;
	}

	bool FileStream::Seek(long long offset, SeekOrigin origin) const noexcept
	{
		long long base = 0;
		switch (origin)
		{
		case SeekOrigin::Begin:   base = 0; break;
		case SeekOrigin::Current: base = static_cast<long long>(pos_); break;
		case SeekOrigin::End:     base = static_cast<long long>(length_); break;
		}
		const long long next = base + offset;
		if (next < 0) { NOX_ASSERT(false, U"FileStream::Seek: 負の位置は無効です"); return false; }
		const nox::uint64 p = static_cast<nox::uint64>(next);
		if (p > length_) { NOX_ASSERT(false, U"FileStream::Seek: 範囲外の位置です"); return false; }
		pos_ = p;
		return true;
	}

	nox::uint64 FileStream::Read(void* dst, nox::uint64 bytes) const
	{
		NOX_ASSERT(CanRead(), U"FileStream::Read: 読み込み不可のストリームです");
		if (!CanRead()) return 0;
		const nox::uint64 avail = (pos_ <= length_) ? (length_ - pos_) : 0;
		const nox::uint64 n = (bytes <= avail) ? bytes : avail;
		if (n > 0)
		{
			NOX_ASSERT(dst != nullptr, U"FileStream::Read: dst が nullptr です");
			::memcpy(dst, buffer_.data() + pos_, static_cast<size_t>(n));
			pos_ += n;
		}
		NOX_ASSERT(n == bytes, U"FileStream::Read: 予期せぬEOF/読み取り不足");
		return n;
	}

	bool FileStream::Write(const void* src, nox::uint64 bytes)
	{
		NOX_ASSERT(CanWrite(), U"FileStream::Write: 書き込み不可のストリームです");
		if (!CanWrite()) return false;
		if (bytes == 0) return true;

		const nox::uint64 need = pos_ + bytes;
		if (need > static_cast<nox::uint64>(buffer_.size()))
		{
			buffer_.resize(static_cast<size_t>(need));
		}
		NOX_ASSERT(src != nullptr, U"FileStream::Write: src が nullptr です");
		::memcpy(buffer_.data() + pos_, src, static_cast<size_t>(bytes));
		pos_ += bytes;
		if (pos_ > length_) length_ = pos_;
		dirty_ = true;
		return true;
	}

	bool FileStream::Flush()
	{
#if defined(_WIN32)
		if (!dirty_) return true;

		DWORD createDisp = OPEN_ALWAYS;
		if (mode_ == FileMode::Create || mode_ == FileMode::Truncate || mode_ == FileMode::CreateNew)
			createDisp = CREATE_ALWAYS;

		HANDLE h = ::CreateFileW(reinterpret_cast<LPCWSTR>(path_u16_.data()),
			GENERIC_WRITE, 0, nullptr, createDisp, FILE_ATTRIBUTE_NORMAL, nullptr);
		NOX_ASSERT(h != INVALID_HANDLE_VALUE, U"FileStream::Flush: CreateFileW に失敗しました");
		if (h == INVALID_HANDLE_VALUE) return false;

		DWORD written = 0;
		const BOOL ok = ::WriteFile(h, buffer_.data(), static_cast<DWORD>(length_), &written, nullptr);
		::CloseHandle(h);
		NOX_ASSERT(ok && written == length_, U"FileStream::Flush: WriteFile に失敗しました");
		if (!(ok && written == length_)) return false;

		dirty_ = false;
		return true;
#else
		NOX_ASSERT(false, U"FileStream::Flush: 非Windowsは未実装です");
		return false;
#endif
	}

	void FileStream::Close() noexcept
	{
		if (dirty_) { (void)Flush(); }
		buffer_.clear();
		length_ = 0;
		pos_ = 0;
		dirty_ = false;
	}

	// =========================
	// BinaryReader（NOX_ASSERT 使用）
	// =========================
	nox::uint8  BinaryReader::ReadByte() { return readPod<nox::uint8>(); }
	nox::int8   BinaryReader::ReadSByte() { return readPod<nox::int8>(); }
	nox::uint16 BinaryReader::ReadUInt16() { return readPod<nox::uint16>(); }
	nox::int16  BinaryReader::ReadInt16() { return readPod<nox::int16>(); }
	nox::uint32 BinaryReader::ReadUInt32() { return readPod<nox::uint32>(); }
	nox::int32  BinaryReader::ReadInt32() { return readPod<nox::int32>(); }
	nox::uint64 BinaryReader::ReadUInt64() { return readPod<nox::uint64>(); }
	nox::int64  BinaryReader::ReadInt64() { return readPod<nox::int64>(); }
	float       BinaryReader::ReadSingle() { return readPod<float>(); }
	double      BinaryReader::ReadDouble() { return readPod<double>(); }

	nox::uint32 BinaryReader::Read7BitEncodedUInt32()
	{
		nox::uint32 value = 0;
		nox::uint32 shift = 0;
		for (int i = 0; i < 5; ++i)
		{
			const nox::uint8 b = ReadByte();
			value |= static_cast<nox::uint32>(b & 0x7Fu) << shift;
			if ((b & 0x80u) == 0) return value;
			shift += 7;
		}
		NOX_ASSERT(false, U"BinaryReader::Read7BitEncodedUInt32: 不正なエンコードです");
		return 0;
	}

	nox::uint64 BinaryReader::Read7BitEncodedUInt64()
	{
		nox::uint64 value = 0;
		nox::uint32 shift = 0;
		for (int i = 0; i < 10; ++i)
		{
			const nox::uint8 b = ReadByte();
			value |= static_cast<nox::uint64>(b & 0x7Fu) << shift;
			if ((b & 0x80u) == 0) return value;
			shift += 7;
		}
		NOX_ASSERT(false, U"BinaryReader::Read7BitEncodedUInt64: 不正なエンコードです");
		return 0;
	}

	nox::uint64 BinaryReader::readPod(void* v, nox::uint64 size)
	{
		const auto got = s_.Read(v, size);
		NOX_ASSERT(got == size, U"");
		return got;
	}
}