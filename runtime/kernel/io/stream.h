//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	stream.h
///	@brief	.NET 風 Stream 抽象と実装（NOX型/アロケータ・例外非依存・NOX_ASSERT使用）
#pragma once
#include	"../advanced_type.h"
#include	"../type_traits/concepts.h"

namespace nox::io
{
	enum class SeekOrigin : nox::uint8
	{
		Begin,
		Current,
		End
	};

	enum class FileMode : nox::uint8
	{
		Open,
		Create,
		OpenOrCreate,
		Truncate,
		Append,
		CreateNew
	};

	enum class FileAccess : nox::uint8
	{
		Read = 0x1,
		Write = 0x2,
		ReadWrite = Read | Write
	};

	class Stream
	{
	public:
		virtual ~Stream() = default;

		virtual bool CanRead()  const noexcept = 0;
		virtual bool CanWrite() const noexcept = 0;
		virtual bool CanSeek()  const noexcept = 0;

		virtual nox::uint64 Length()   const noexcept = 0;
		virtual nox::uint64 Position() const noexcept = 0;

		virtual bool        Seek(long long offset, SeekOrigin origin) const noexcept = 0;
		virtual nox::uint64 Read(void* dst, nox::uint64 bytes) const = 0;

		virtual bool Write(const void* src, nox::uint64 bytes) = 0;
		virtual bool Flush() = 0;
		virtual void Close() noexcept = 0;

		inline bool Eof() const noexcept { return Position() >= Length(); }
		inline nox::uint64 Remaining() const noexcept
		{
			const auto len = Length();
			const auto pos = Position();
			return pos <= len ? (len - pos) : 0;
		}
		inline bool Rewind() const noexcept { return Seek(0, SeekOrigin::Begin); }

		template<nox::concepts::Trivial T>
		inline bool Read(T& out) const
		{
			const auto got = this->Read(&out, static_cast<nox::uint64>(sizeof(T)));
			NOX_ASSERT(got == sizeof(T));
			return got == sizeof(T);
		}

		template<class T>
		inline bool Write(const T& v)
		{
			const bool ok = Write(&v, static_cast<nox::uint64>(sizeof(T)));
			NOX_ASSERT(ok);
			return ok;
		}
	};

	// 所有/非所有のメモリストリーム
	class MemoryStream final : public Stream
	{
	public:
		MemoryStream() = default;
		explicit MemoryStream(nox::Vector<nox::uint8> buffer, bool writable = true) noexcept;
		MemoryStream(const nox::uint8* data, nox::uint64 size) noexcept;

		bool CanRead()  const noexcept override { return (view_ != nullptr) || (owned_size_ != 0); }
		bool CanWrite() const noexcept override { return writable_ && (view_ == nullptr); }
		bool CanSeek()  const noexcept override { return true; }

		nox::uint64 Length()   const noexcept override { return size_; }
		nox::uint64 Position() const noexcept override { return pos_; }

		bool        Seek(long long offset, SeekOrigin origin) const noexcept override;
		nox::uint64 Read(void* dst, nox::uint64 bytes) const override;
		bool        Write(const void* src, nox::uint64 bytes) override;
		bool        Flush() override { return true; }
		void        Close() noexcept override;

		const nox::Vector<nox::uint8>& Buffer() const noexcept { return owned_; }
		nox::Vector<nox::uint8>        MoveBuffer() noexcept;

	private:
		nox::Vector<nox::uint8> owned_{};
		const nox::uint8* view_{ nullptr };
		nox::uint64             size_{ 0 };
		mutable nox::uint64     pos_{ 0 };
		nox::uint64             owned_size_{ 0 };
		bool                    writable_{ true };
	};

	// Win32 OS ファイルストリーム（std未使用）
	class FileStream final : public Stream
	{
	public:
		FileStream() = default;
		FileStream(std::u32string_view path, FileMode mode, FileAccess access) { Open(path, mode, access); }
		~FileStream() override { Close(); }

		bool Open(std::u32string_view path, FileMode mode, FileAccess access);

		bool CanRead()  const noexcept override { return (static_cast<nox::uint8>(access_) & static_cast<nox::uint8>(FileAccess::Read)) != 0; }
		bool CanWrite() const noexcept override { return (static_cast<nox::uint8>(access_) & static_cast<nox::uint8>(FileAccess::Write)) != 0; }
		bool CanSeek()  const noexcept override { return true; }

		nox::uint64 Length()   const noexcept override { return length_; }
		nox::uint64 Position() const noexcept override { return pos_; }

		bool        Seek(long long offset, SeekOrigin origin) const noexcept override;
		nox::uint64 Read(void* dst, nox::uint64 bytes) const override;
		bool        Write(const void* src, nox::uint64 bytes) override;
		bool        Flush() override;
		void        Close() noexcept override;

	private:
		// 読み込みは全読み込み（buffer_）、書き込みはバッファへ反映し Flush/Close で出力
		nox::Vector<nox::uint8> buffer_;
		nox::uint64             length_{ 0 };
		mutable nox::uint64     pos_{ 0 };
		FileMode                mode_{ FileMode::Open };
		FileAccess              access_{ FileAccess::Read };
		nox::U16String          path_u16_;
		bool                    dirty_{ false };
	};

	// バイナリリーダ（主に .NET の BinaryReader 相当、NOX_ASSERT で失敗通知）
	class BinaryReader
	{
	public:
		explicit BinaryReader(const Stream& s) noexcept : s_(s) {}

		const Stream& BaseStream() const noexcept { return s_; }

		nox::uint8   ReadByte();
		nox::int8    ReadSByte();
		nox::uint16  ReadUInt16();
		nox::int16   ReadInt16();
		nox::uint32  ReadUInt32();
		nox::int32   ReadInt32();
		nox::uint64  ReadUInt64();
		nox::int64   ReadInt64();
		float        ReadSingle();
		double       ReadDouble();

		/*nox::uint64 Read(void* dst, nox::uint64 bytes) 
		{ 
			const auto got = s_.Read(dst, bytes); 
			NOX_ASSERT(got == bytes, U"");
			return got; 
		}*/

		// 7bit 圧縮整数（.NET互換）
		nox::uint32 Read7BitEncodedUInt32();
		nox::uint64 Read7BitEncodedUInt64();

	private:
		template<class T>
		inline T readPod()
		{
			T v{};
			return readPod(&v, sizeof(T));
		}

		inline nox::uint64 readPod(void* v, nox::uint64 size);

	private:
		const Stream& s_;
	};
}