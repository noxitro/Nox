//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_reader.cpp
///	@brief	socket_stream_reader
#include	"pch.h"
#include	"socket_stream_reader.h"

#include	"editor_remote_server.h"
#include	"attribute_common.h"
#include	"socket_stream_utility.h"
#include	"../managed_object.h"

nox::uint64 nox::dev::editor_remote::SocketStreamReader::ReadLength()
{
	// LEB128（unsigned）をデコードして uint64 を返す。
// 注意: 呼び出し側は事前に十分なバイトがバッファにあることを保証するか、
// 	   // バッファに少なくとも1バイトあることを確認
	NOX_ASSERT(GetReceivedSize() > 0, u"SocketStreamReaderの受信バッファが不足しています (ReadLength)");
//       ここでアサートが発生することを許容すること。
	nox::uint64 value = 0;
	nox::uint32 shift = 0;

	// LEB128 for 64bit は最大 10 バイト
	for (nox::int32 i = 0; i < 10; ++i)
	{
		// 現在の読み出し位置からバイトを取得（リングバッファのラップを考慮）
		const nox::uint32 idx = read_pos_ & (k_buffer_size - 1);
		const nox::uint8 byte = buffer_[idx];

		value |= static_cast<nox::uint64>(byte & 0x7Fu) << shift;

		// 読み出し位置を進める（モジュロ）
		read_pos_ = (read_pos_ + 1) & (k_buffer_size - 1);

		// MSB が 0 なら終端
		if ((byte & 0x80u) == 0)
		{
			return value;
		}

		shift += 7;
	}

	// ここに到達するのはフォーマット異常（過剰なバイト）
	NOX_ASSERT(false, u"LEB128 デコードエラー: 長すぎるエンコーディング");
	return 0;
}

void nox::dev::editor_remote::SocketStreamReader::AddReceiveBuffer(std::span<const nox::uint8> buffer)
{
	//	readとAddReceiveBufferはマルチスレッドで呼ばれるので、書き込み順に気を付ける
	//	排他制御はしない方針

	const nox::uint32 write_size = static_cast<nox::uint32>(buffer.size());

	//	書き込み可能サイズ
    const nox::uint32 free_size = k_buffer_size - GetReceivedSize() - 1;

	//	バッファオーバーチェック
	NOX_ASSERT(write_size <= free_size, u"SocketStreamReaderの受信バッファがオーバーフローしました");

	//	先に書き込み、そのあとにrecv_pos_を更新する
	const nox::uint32 received_pos = (recv_pos_ + write_size) & (k_buffer_size - 1);

	if (recv_pos_ + write_size <= k_buffer_size)
	{
		//	一回のコピーで済む場合
		nox::memory::Copy(buffer_.data() + recv_pos_, buffer.data(), write_size);
	}
	else
	{
		//	分割してコピーする場合
		const nox::uint32 first_copy_size = k_buffer_size - recv_pos_;
		nox::memory::Copy(buffer_.data() + recv_pos_, buffer.data(), first_copy_size);
		const nox::uint32 second_copy_size = write_size - first_copy_size;
		nox::memory::Copy(buffer_.data(), buffer.data() + first_copy_size, second_copy_size);
	}

	recv_pos_ = received_pos;
}

void	nox::dev::editor_remote::SocketStreamReader::ReadBytes(std::span<nox::uint8> dest)
{
	const nox::uint32 need = static_cast<nox::uint32>(dest.size());

	//	バッファチェック
	NOX_ASSERT(need <= GetReceivedSize(), u"SocketStreamReaderの受信バッファが不足しています");

	//const nox::uint32 recv_pos = recv_pos_;

	if (read_pos_ + need <= k_buffer_size)
	{
		//	一回のコピーで済む場合
		nox::memory::Copy(dest.data(), buffer_.data() + read_pos_, need);
	}
	else
	{
		//	分割してコピーする場合
		const nox::uint32 first_copy_size = k_buffer_size - read_pos_;
		nox::memory::Copy(dest.data(), buffer_.data() + read_pos_, first_copy_size);
		const nox::uint32 second_copy_size = need - first_copy_size;
		nox::memory::Copy(dest.data() + first_copy_size, buffer_.data(), second_copy_size);
	}

	read_pos_ = (read_pos_ + need) & (k_buffer_size - 1);
}

void nox::dev::editor_remote::SocketStreamReader::Read(std::span<nox::uint8> dest)
{
	const nox::uint64 length = ReadLength();
	NOX_ASSERT(length <= static_cast<nox::uint64>(dest.size()), u"SocketStreamReader::ReadString: バッファサイズオーバー");
	this->ReadBytes(std::span(dest.data(), static_cast<nox::uint32>(length)));
}

void nox::dev::editor_remote::SocketStreamReader::Read(std::span<nox::char8> dest)
{
	const nox::uint64 length = ReadLength();
	NOX_ASSERT(length <= static_cast<nox::uint64>(dest.size()), u"SocketStreamReader::ReadString: バッファサイズオーバー length:{0}, buffer_size:{1}", length, dest.size());
	this->ReadBytes(std::span<nox::uint8>(reinterpret_cast<nox::uint8*>(dest.data()), static_cast<nox::uint32>(length)));
}

nox::StlU8String nox::dev::editor_remote::SocketStreamReader::ReadString()
{
	const nox::uint64 length = ReadLength();
	nox::StlU8String result(static_cast<std::size_t>(length), u'0');
	this->ReadBytes(std::span<nox::uint8>(reinterpret_cast<nox::uint8*>(result.data()), static_cast<nox::uint32>(length)));
	return result;
}

void nox::dev::editor_remote::SocketStreamReader::Read(nox::IntrusivePtr<nox::ManagedObject>& value)
{
	//	remote instance idを読み取る
	nox::int64 remote_instance_id;
	this->Read(remote_instance_id);

	if (remote_instance_id != 0)
	{
		//	remote instance idが0でないなら、リモートインスタンスを探して返す
		nox::Object* remote_instance = server_.FindRemoteInstance(remote_instance_id);
		NOX_ASSERT(remote_instance != nullptr, u"リモートインスタンスが見つかりませんでした remote_instance_id:{0}", remote_instance_id);
		nox::ManagedObject* obj = nox::reflection::AsCast<nox::ManagedObject*>(remote_instance);
		NOX_ASSERT(obj != nullptr, u"リモートインスタンスはManagedObjectを継承している必要があります remote_instance_id:{0}", remote_instance_id);

		value.Reset(obj);
		return;
	}

	//	instanceIdが0なら、editor側で作成されたインスタンス
	//	型名とプロパティバッファが入っているので読み取る
	const nox::reflection::ClassInfo& class_info = [this]() -> const nox::reflection::ClassInfo&
		{
			std::array<nox::char8, nox::reflection::k_max_fqn_length> type_name_buffer{ 0 };
			const std::u8string_view type_name = this->ReadString(type_name_buffer);
			return nox::util::Deref(nox::reflection::FindClassInfo(type_name));
		}();
		
	NOX_ASSERT(class_info.IsSubclassOf<nox::reflection::ReflectionObject>(), u"ReflectionObject継承のクラスではありません:{0}", class_info.GetFullName());
	nox::reflection::ReflectionObject* obj = static_cast<nox::reflection::ReflectionObject*>(class_info.GetType().CreateObject());
	NOX_ASSERT(obj != nullptr, u"インスタンスの作成に失敗しました:{0}", class_info.GetFullName());

	const auto variable_list = class_info.GetVariableList();

	for (const nox::reflection::VariableInfo& variable_info : variable_list)
	{
		//	シリアライズ対象か
		if (nox::dev::editor_remote::IsRemoteVariable(variable_info) == false)
		{
			continue;
		}

		const nox::reflection::Type& variable_type = variable_info.GetType();
		switch (variable_type.GetTypeKind())
		{
		case nox::reflection::TypeKind::Bool:
		{
			bool v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::Int8:
		{
			nox::int8 v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::UInt8:
		{
			nox::uint8 v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::Int16:
		{
			nox::int16 v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::UInt16:
		{
			nox::uint16 v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::Int32:
		{
			nox::int32 v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::UInt32:
		{
			nox::uint32 v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::Int64:
		{
			nox::int64 v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::UInt64:
		{
			nox::uint64 v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::Float:
		{
			nox::float_t v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::Double:
		{
			nox::double_t v;
			this->Read(v);
			variable_info.SetValue(value, v);
		}
		break;
		case nox::reflection::TypeKind::Class:
		{
			const nox::reflection::ClassInfo* const variable_type_class_info = variable_type.GetUserDefinedCompoundTypeInfo();
			if (variable_type_class_info != nullptr)
			{
				if (variable_type_class_info->IsBaseOf(nox::reflection::Typeof<nox::reflection::ReflectionObject>()) == true)
				{
					NOX_ASSERT(false, u"未実装");
					//	ReflectionObjectを継承したクラスか？
				}
				else if (variable_type.IsTypeAttributeFlag(nox::reflection::TypeAttributeFlag::TrivialCopyable))
				{
					//	トリビアルなクラス
					NOX_ASSERT(false, u"未実装");
				}
				else
				{
					NOX_ASSERT(false, u"SocketStreamReader::Read: クラス型ですがリフレクション非対応の型です");
				}
			}
			else
			{
				NOX_ASSERT(false, u"SocketStreamReader::Read: クラス型ですがリフレクション非対応の型です");
			}
		}
		break;

		default:
			NOX_ASSERT(false, u"SocketStreamReader::Read: 対応していない型です");
			break;
		}
	}
}

bool nox::dev::editor_remote::SocketStreamReader::CanReadBody()const noexcept
{
	// 先頭のパケット全長(LEB128)を先読み
	nox::uint64 packet_size = 0;
	nox::uint32 header_bytes = 0;
	nox::uint32 shift = 0;

	// LEB128 for 64bit は最大 10 バイト
	for (nox::uint32 i = 0; i < 10; ++i)
	{
		if (i >= GetReceivedSize())
		{
			return false;
		}
		const nox::uint32 idx = (read_pos_ + i) & (k_buffer_size - 1);
		const nox::uint8 byte = buffer_[idx];
		packet_size |= static_cast<nox::uint64>(byte & 0x7Fu) << shift;
		shift += 7;
		++header_bytes;

		if ((byte & 0x80u) == 0)
		{
			// ヘッダ + パケット全体が揃っているか
           return packet_size <= k_buffer_size && GetReceivedSize() >= header_bytes + static_cast<nox::uint32>(packet_size);
		}
	}
	return false;
}

void nox::dev::editor_remote::SocketStreamReader::SkipHeader()
{
	// 先頭のパケット全長(LEB128)を読み飛ばす
	for (nox::uint32 i = 0; i < 10; ++i)
	{
		NOX_ASSERT(i < GetReceivedSize(), u"SocketStreamReaderの受信バッファが不足しています (SkipHeader)");
		const nox::uint32 idx = (read_pos_ + i) & (k_buffer_size - 1);
		const nox::uint8 byte = buffer_[idx];
		read_pos_ = (read_pos_ + 1) & (k_buffer_size - 1);
		if ((byte & 0x80u) == 0)
		{
			return;
		}
	}
	NOX_ASSERT(false, u"LEB128 デコードエラー: 長すぎるエンコーディング");
}