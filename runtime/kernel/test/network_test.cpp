//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network_test.cpp
///	@brief	network (ソケット層) のテスト。ループバックだけを使う

#include "pch.h"
#include <array>
#include "../algorithm.h"
#include "../network.h"

namespace
{
	namespace network = nox::os::network;
	using network::Error;
	using network::EndPoint;
	using network::IpFamily;
	using network::Protocol;
	using network::Socket;
	using network::SocketMode;

	//	テストの途中で失敗してもソケットを閉じる
	struct ScopedSocket
	{
		Socket socket;

		ScopedSocket() = default;
		ScopedSocket(const ScopedSocket&) = delete;
		ScopedSocket& operator=(const ScopedSocket&) = delete;
		~ScopedSocket() { (void)network::Close(socket); }
	};

	class NetworkTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ASSERT_EQ(network::Initialize(), Error::None);
		}

		void TearDown() override
		{
			network::Finalize();
		}

		static void Create(const IpFamily family, const Protocol protocol, ScopedSocket& out)
		{
			const auto created = network::CreateSocket(family, protocol);
			ASSERT_TRUE(created.has_value());
			out.socket = *created;
		}

		//	ループバックで TCP の接続を 1 本作る
		static void MakeTcpPair(ScopedSocket& listener, ScopedSocket& client, ScopedSocket& server, const SocketMode server_mode = SocketMode::Blocking)
		{
			ASSERT_NO_FATAL_FAILURE(Create(IpFamily::IPv4, Protocol::TCP, listener));
			ASSERT_EQ(network::Bind(listener.socket, EndPoint::IPv4Loopback(0)), Error::None);
			ASSERT_EQ(network::Listen(listener.socket, 1), Error::None);

			const auto bound = network::GetLocalEndPoint(listener.socket);
			ASSERT_TRUE(bound.has_value());
			ASSERT_EQ(bound->family, IpFamily::IPv4);
			ASSERT_NE(bound->port, 0);

			ASSERT_NO_FATAL_FAILURE(Create(IpFamily::IPv4, Protocol::TCP, client));
			ASSERT_EQ(network::Connect(client.socket, *bound), Error::None);

			const auto accepted = network::Accept(listener.socket, server_mode);
			ASSERT_TRUE(accepted.has_value());
			server.socket = accepted->socket;
			ASSERT_TRUE(server.socket.IsValid());

			//	相手のアドレスは accept が返したもの
			const auto client_local = network::GetLocalEndPoint(client.socket);
			ASSERT_TRUE(client_local.has_value());
			EXPECT_EQ(accepted->remote, *client_local);
		}
	};
}

TEST(NetworkEndPoint, Factories)
{
	static_assert(EndPoint::IPv4Loopback(80).address[0] == 127);
	static_assert(EndPoint::IPv4Loopback(80).address[3] == 1);
	static_assert(EndPoint::IPv6Loopback(0).address[15] == 1);
	static_assert(EndPoint::IPv4Any(0) == EndPoint::MakeIPv4(0, 0, 0, 0, 0));
	static_assert(Socket{}.IsValid() == false);
	static_assert(nox::util::BitOr(network::SendFlag::Oob, network::SendFlag::DontRoute) != network::SendFlag::None);

	EXPECT_EQ(EndPoint::IPv4Loopback(8080).port, 8080);
	EXPECT_EQ(EndPoint::IPv6Any(1).family, IpFamily::IPv6);
}

TEST_F(NetworkTest, ParseAddress)
{
	const auto ipv4 = network::ParseAddress("127.0.0.1", 1234);
	ASSERT_TRUE(ipv4.has_value());
	EXPECT_EQ(*ipv4, EndPoint::IPv4Loopback(1234));

	const auto ipv6 = network::ParseAddress("::1", 5);
	ASSERT_TRUE(ipv6.has_value());
	EXPECT_EQ(*ipv6, EndPoint::IPv6Loopback(5));

	EXPECT_EQ(network::ParseAddress("", 0).error(), Error::InvalidArgument);
	EXPECT_EQ(network::ParseAddress("256.0.0.1", 0).error(), Error::InvalidArgument);
	EXPECT_EQ(network::ParseAddress("localhost", 0).error(), Error::InvalidArgument);
}

TEST_F(NetworkTest, CreateSocketRejectsUnknownFamily)
{
	const auto created = network::CreateSocket(IpFamily::Unknown, Protocol::TCP);
	ASSERT_FALSE(created.has_value());
	EXPECT_EQ(created.error(), Error::InvalidArgument);
}

TEST_F(NetworkTest, CloseInvalidatesSocket)
{
	auto created = network::CreateSocket(IpFamily::IPv4, Protocol::TCP);
	ASSERT_TRUE(created.has_value());
	Socket socket = *created;

	EXPECT_EQ(network::Close(socket), Error::None);
	EXPECT_FALSE(socket.IsValid());

	//	閉じたものをもう一度閉じても何もしない
	EXPECT_EQ(network::Close(socket), Error::None);
}

TEST_F(NetworkTest, TcpSendReceive)
{
	ScopedSocket listener;
	ScopedSocket client;
	ScopedSocket server;
	ASSERT_NO_FATAL_FAILURE(MakeTcpPair(listener, client, server));
	ASSERT_EQ(network::SetNoDelay(client.socket, true), Error::None);

	const std::array<nox::uint8, 5> sent = { 'h', 'e', 'l', 'l', 'o' };
	const network::IoResult send_result = network::Send(client.socket, sent);
	ASSERT_TRUE(send_result.IsOk());
	ASSERT_EQ(send_result.bytes, 5);

	std::array<nox::uint8, 5> received = {};
	const network::IoResult receive_result = network::Receive(server.socket, received, network::ReceiveFlag::WaitAll);
	ASSERT_TRUE(receive_result.IsOk());
	ASSERT_EQ(receive_result.bytes, 5);
	EXPECT_EQ(received, sent);

	//	送信を閉じると、相手の Receive は 0 バイトで成功する
	ASSERT_EQ(network::Shutdown(client.socket, network::ShutdownMode::Send), Error::None);
	const network::IoResult closed_result = network::Receive(server.socket, received);
	EXPECT_TRUE(closed_result.IsOk());
	EXPECT_EQ(closed_result.bytes, 0);
}

TEST_F(NetworkTest, AcceptAppliesNonBlockingMode)
{
	ScopedSocket listener;
	ScopedSocket client;
	ScopedSocket server;
	ASSERT_NO_FATAL_FAILURE(MakeTcpPair(listener, client, server, SocketMode::NonBlocking));

	//	ノンブロッキングで受けたので、何も届いていなければ待たずに WouldBlock が返る
	std::array<nox::uint8, 4> buffer = {};
	const network::IoResult result = network::Receive(server.socket, buffer);
	EXPECT_EQ(result.error, Error::WouldBlock);
	EXPECT_EQ(result.bytes, 0);
}

TEST_F(NetworkTest, AcceptOnNonBlockingListener)
{
	ScopedSocket listener;
	ASSERT_NO_FATAL_FAILURE(Create(IpFamily::IPv4, Protocol::TCP, listener));
	ASSERT_EQ(network::Bind(listener.socket, EndPoint::IPv4Loopback(0)), Error::None);
	ASSERT_EQ(network::Listen(listener.socket, 1), Error::None);
	ASSERT_EQ(network::SetNonBlocking(listener.socket, true), Error::None);

	//	誰も来ていなければ待たずに WouldBlock
	const auto none = network::Accept(listener.socket, SocketMode::Blocking);
	ASSERT_FALSE(none.has_value());
	EXPECT_EQ(none.error(), Error::WouldBlock);

	const auto bound = network::GetLocalEndPoint(listener.socket);
	ASSERT_TRUE(bound.has_value());
	ScopedSocket client;
	ASSERT_NO_FATAL_FAILURE(Create(IpFamily::IPv4, Protocol::TCP, client));
	ASSERT_EQ(network::Connect(client.socket, *bound), Error::None);

	//	接続が届いたら Readable になる
	std::array<network::PollEntry, 1> entries = {};
	entries[0].socket = listener.socket;
	entries[0].requested = network::PollEvent::Readable;
	const auto ready = network::Poll(entries, 5000);
	ASSERT_TRUE(ready.has_value());
	ASSERT_EQ(*ready, 1);

	const auto accepted = network::Accept(listener.socket, SocketMode::Blocking);
	ASSERT_TRUE(accepted.has_value());
	ScopedSocket server;
	server.socket = accepted->socket;

	const std::array<nox::uint8, 2> sent = { 1, 2 };
	ASSERT_TRUE(network::Send(client.socket, sent).IsOk());
	std::array<nox::uint8, 2> received = {};
	const network::IoResult result = network::Receive(server.socket, received, network::ReceiveFlag::WaitAll);
	ASSERT_TRUE(result.IsOk());
	EXPECT_EQ(result.bytes, 2);
}

TEST_F(NetworkTest, PollReadable)
{
	ScopedSocket listener;
	ScopedSocket client;
	ScopedSocket server;
	ASSERT_NO_FATAL_FAILURE(MakeTcpPair(listener, client, server));

	std::array<network::PollEntry, 1> entries = {};
	entries[0].socket = server.socket;
	entries[0].requested = network::PollEvent::Readable;

	const auto none = network::Poll(entries, 0);
	ASSERT_TRUE(none.has_value());
	EXPECT_EQ(*none, 0);
	EXPECT_EQ(entries[0].returned, network::PollEvent::None);

	const std::array<nox::uint8, 1> data = { 42 };
	ASSERT_TRUE(network::Send(client.socket, data).IsOk());

	const auto ready = network::Poll(entries, 5000);
	ASSERT_TRUE(ready.has_value());
	EXPECT_EQ(*ready, 1);
	EXPECT_TRUE(nox::util::IsBitAnd(entries[0].returned, network::PollEvent::Readable));
}

TEST_F(NetworkTest, PollArguments)
{
	EXPECT_EQ(network::Poll({}, 0).error(), Error::InvalidArgument);

	//	無効なソケットは待たずに Invalid が返る
	std::array<network::PollEntry, 1> entries = {};
	entries[0].requested = network::PollEvent::Readable;
	const auto ready = network::Poll(entries, -1);
	ASSERT_TRUE(ready.has_value());
	EXPECT_EQ(*ready, 1);
	EXPECT_EQ(entries[0].returned, network::PollEvent::Invalid);
}

TEST_F(NetworkTest, UdpSendToReceiveFrom)
{
	ScopedSocket sender;
	ScopedSocket receiver;
	ASSERT_NO_FATAL_FAILURE(Create(IpFamily::IPv4, Protocol::UDP, sender));
	ASSERT_NO_FATAL_FAILURE(Create(IpFamily::IPv4, Protocol::UDP, receiver));
	ASSERT_EQ(network::Bind(sender.socket, EndPoint::IPv4Loopback(0)), Error::None);
	ASSERT_EQ(network::Bind(receiver.socket, EndPoint::IPv4Loopback(0)), Error::None);

	const auto sender_address = network::GetLocalEndPoint(sender.socket);
	const auto receiver_address = network::GetLocalEndPoint(receiver.socket);
	ASSERT_TRUE(sender_address.has_value());
	ASSERT_TRUE(receiver_address.has_value());

	const std::array<nox::uint8, 3> sent = { 1, 2, 3 };
	const network::IoResult send_result = network::SendTo(sender.socket, sent, *receiver_address);
	ASSERT_TRUE(send_result.IsOk());
	ASSERT_EQ(send_result.bytes, 3);

	std::array<nox::uint8, 8> received = {};
	const network::ReceiveFromResult receive_result = network::ReceiveFrom(receiver.socket, received);
	ASSERT_TRUE(receive_result.IsOk());
	ASSERT_EQ(receive_result.bytes, 3);
	EXPECT_EQ(received[0], 1);
	EXPECT_EQ(received[2], 3);
	EXPECT_EQ(receive_result.source, *sender_address);
}

TEST_F(NetworkTest, UdpTruncationIsReported)
{
	ScopedSocket sender;
	ScopedSocket receiver;
	ASSERT_NO_FATAL_FAILURE(Create(IpFamily::IPv4, Protocol::UDP, sender));
	ASSERT_NO_FATAL_FAILURE(Create(IpFamily::IPv4, Protocol::UDP, receiver));
	ASSERT_EQ(network::Bind(receiver.socket, EndPoint::IPv4Loopback(0)), Error::None);

	const auto receiver_address = network::GetLocalEndPoint(receiver.socket);
	ASSERT_TRUE(receiver_address.has_value());

	const std::array<nox::uint8, 8> sent = { 1, 2, 3, 4, 5, 6, 7, 8 };
	ASSERT_TRUE(network::SendTo(sender.socket, sent, *receiver_address).IsOk());

	//	受信バッファより大きいデータグラムは、入った分だけ書いて MessageTooLong を返す
	std::array<nox::uint8, 4> received = {};
	const network::ReceiveFromResult result = network::ReceiveFrom(receiver.socket, received);
	EXPECT_EQ(result.error, Error::MessageTooLong);
	EXPECT_EQ(result.bytes, 4);
	EXPECT_EQ(received[3], 4);
}

TEST_F(NetworkTest, Ipv6Loopback)
{
	ScopedSocket listener;
	const auto created = network::CreateSocket(IpFamily::IPv6, Protocol::TCP);
	if (created.has_value() == false)
	{
		GTEST_SKIP() << "IPv6 のソケットを作れない環境";
	}
	listener.socket = *created;
	ASSERT_EQ(network::SetIPv6Only(listener.socket, true), Error::None);

	const Error bind_error = network::Bind(listener.socket, EndPoint::IPv6Loopback(0));
	if (bind_error == Error::AddressNotAvailable)
	{
		GTEST_SKIP() << "IPv6 のループバックが無い環境";
	}
	ASSERT_EQ(bind_error, Error::None);

	const auto bound = network::GetLocalEndPoint(listener.socket);
	ASSERT_TRUE(bound.has_value());
	EXPECT_EQ(bound->family, IpFamily::IPv6);
	EXPECT_EQ(bound->address[15], 1);
	EXPECT_NE(bound->port, 0);
}
