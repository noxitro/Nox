//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network_test.cpp
///	@brief	network (ソケット層) のテスト。ループバックだけを使う

#include "pch.h"
#include <array>
#include "../network.h"

namespace
{
	namespace network = nox::os::network;
	using network::Error;
	using network::EndPoint;
	using network::IpFamily;
	using network::Protocol;
	using network::Socket;

	//	テストの途中で失敗してもソケットを閉じる
	struct ScopedSocket
	{
		Socket socket;

		ScopedSocket() = default;
		ScopedSocket(const ScopedSocket&) = delete;
		ScopedSocket& operator=(const ScopedSocket&) = delete;
		~ScopedSocket() { network::Close(socket); }
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

		//	ループバックで TCP の接続を 1 本作る
		static void MakeTcpPair(ScopedSocket& listener, ScopedSocket& client, ScopedSocket& server)
		{
			ASSERT_EQ(network::CreateSocket(IpFamily::IPv4, Protocol::Tcp, listener.socket), Error::None);
			ASSERT_EQ(network::Bind(listener.socket, EndPoint::IPv4Loopback(0)), Error::None);
			ASSERT_EQ(network::Listen(listener.socket, 1), Error::None);

			EndPoint bound;
			ASSERT_EQ(network::GetLocalEndPoint(listener.socket, bound), Error::None);
			ASSERT_EQ(bound.family, IpFamily::IPv4);
			ASSERT_NE(bound.port, 0);

			ASSERT_EQ(network::CreateSocket(IpFamily::IPv4, Protocol::Tcp, client.socket), Error::None);
			ASSERT_EQ(network::Connect(client.socket, bound), Error::None);

			EndPoint remote;
			ASSERT_EQ(network::Accept(listener.socket, server.socket, &remote), Error::None);
			ASSERT_TRUE(server.socket.IsValid());

			EndPoint client_local;
			ASSERT_EQ(network::GetLocalEndPoint(client.socket, client_local), Error::None);
			EXPECT_EQ(remote, client_local);
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

	EXPECT_EQ(EndPoint::IPv4Loopback(8080).port, 8080);
	EXPECT_EQ(EndPoint::IPv6Any(1).family, IpFamily::IPv6);
}

TEST_F(NetworkTest, ParseAddress)
{
	EndPoint end_point;
	ASSERT_EQ(network::ParseAddress("127.0.0.1", 1234, end_point), Error::None);
	EXPECT_EQ(end_point, EndPoint::IPv4Loopback(1234));

	ASSERT_EQ(network::ParseAddress("::1", 5, end_point), Error::None);
	EXPECT_EQ(end_point, EndPoint::IPv6Loopback(5));

	EXPECT_EQ(network::ParseAddress("", 0, end_point), Error::InvalidArgument);
	EXPECT_EQ(network::ParseAddress("256.0.0.1", 0, end_point), Error::InvalidArgument);
	EXPECT_EQ(network::ParseAddress("localhost", 0, end_point), Error::InvalidArgument);
	EXPECT_EQ(end_point.family, IpFamily::Unknown);
}

TEST_F(NetworkTest, CloseInvalidatesSocket)
{
	Socket socket;
	ASSERT_EQ(network::CreateSocket(IpFamily::IPv4, Protocol::Tcp, socket), Error::None);
	ASSERT_TRUE(socket.IsValid());

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
	ASSERT_EQ(send_result.error, Error::None);
	ASSERT_EQ(send_result.bytes, 5);

	std::array<nox::uint8, 5> received = {};
	const network::IoResult receive_result = network::Receive(server.socket, received, network::ReceiveFlag::WaitAll);
	ASSERT_EQ(receive_result.error, Error::None);
	ASSERT_EQ(receive_result.bytes, 5);
	EXPECT_EQ(received, sent);

	//	送信を閉じると、相手の Receive は 0 バイトで成功する
	ASSERT_EQ(network::Shutdown(client.socket, network::ShutdownMode::Send), Error::None);
	const network::IoResult closed_result = network::Receive(server.socket, received);
	EXPECT_EQ(closed_result.error, Error::None);
	EXPECT_EQ(closed_result.bytes, 0);
}

TEST_F(NetworkTest, NonBlockingReceiveWouldBlock)
{
	ScopedSocket listener;
	ScopedSocket client;
	ScopedSocket server;
	ASSERT_NO_FATAL_FAILURE(MakeTcpPair(listener, client, server));
	ASSERT_EQ(network::SetNonBlocking(server.socket, true), Error::None);

	std::array<nox::uint8, 4> buffer = {};
	const network::IoResult result = network::Receive(server.socket, buffer);
	EXPECT_EQ(result.error, Error::WouldBlock);
	EXPECT_EQ(result.bytes, 0);
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

	nox::int32 ready_count = -1;
	ASSERT_EQ(network::Poll(entries, 0, ready_count), Error::None);
	EXPECT_EQ(ready_count, 0);
	EXPECT_EQ(entries[0].returned, network::PollEvent::None);

	const std::array<nox::uint8, 1> data = { 42 };
	ASSERT_EQ(network::Send(client.socket, data).error, Error::None);

	ASSERT_EQ(network::Poll(entries, 5000, ready_count), Error::None);
	EXPECT_EQ(ready_count, 1);
	EXPECT_TRUE(network::HasAny(entries[0].returned, network::PollEvent::Readable));
}

TEST_F(NetworkTest, PollArguments)
{
	nox::int32 ready_count = -1;
	EXPECT_EQ(network::Poll({}, 0, ready_count), Error::InvalidArgument);

	//	無効なソケットは待たずに Invalid が返る
	std::array<network::PollEntry, 1> entries = {};
	entries[0].requested = network::PollEvent::Readable;
	ASSERT_EQ(network::Poll(entries, -1, ready_count), Error::None);
	EXPECT_EQ(ready_count, 1);
	EXPECT_EQ(entries[0].returned, network::PollEvent::Invalid);
}

TEST_F(NetworkTest, UdpSendToReceiveFrom)
{
	ScopedSocket sender;
	ScopedSocket receiver;
	ASSERT_EQ(network::CreateSocket(IpFamily::IPv4, Protocol::Udp, sender.socket), Error::None);
	ASSERT_EQ(network::CreateSocket(IpFamily::IPv4, Protocol::Udp, receiver.socket), Error::None);
	ASSERT_EQ(network::Bind(sender.socket, EndPoint::IPv4Loopback(0)), Error::None);
	ASSERT_EQ(network::Bind(receiver.socket, EndPoint::IPv4Loopback(0)), Error::None);

	EndPoint sender_address;
	EndPoint receiver_address;
	ASSERT_EQ(network::GetLocalEndPoint(sender.socket, sender_address), Error::None);
	ASSERT_EQ(network::GetLocalEndPoint(receiver.socket, receiver_address), Error::None);

	const std::array<nox::uint8, 3> sent = { 1, 2, 3 };
	const network::IoResult send_result = network::SendTo(sender.socket, sent, receiver_address);
	ASSERT_EQ(send_result.error, Error::None);
	ASSERT_EQ(send_result.bytes, 3);

	std::array<nox::uint8, 8> received = {};
	EndPoint source;
	const network::IoResult receive_result = network::ReceiveFrom(receiver.socket, received, &source);
	ASSERT_EQ(receive_result.error, Error::None);
	ASSERT_EQ(receive_result.bytes, 3);
	EXPECT_EQ(received[0], 1);
	EXPECT_EQ(received[2], 3);
	EXPECT_EQ(source, sender_address);
}

TEST_F(NetworkTest, UdpTruncationIsReported)
{
	ScopedSocket sender;
	ScopedSocket receiver;
	ASSERT_EQ(network::CreateSocket(IpFamily::IPv4, Protocol::Udp, sender.socket), Error::None);
	ASSERT_EQ(network::CreateSocket(IpFamily::IPv4, Protocol::Udp, receiver.socket), Error::None);
	ASSERT_EQ(network::Bind(receiver.socket, EndPoint::IPv4Loopback(0)), Error::None);

	EndPoint receiver_address;
	ASSERT_EQ(network::GetLocalEndPoint(receiver.socket, receiver_address), Error::None);

	const std::array<nox::uint8, 8> sent = { 1, 2, 3, 4, 5, 6, 7, 8 };
	ASSERT_EQ(network::SendTo(sender.socket, sent, receiver_address).error, Error::None);

	//	受信バッファより大きいデータグラムは、入った分だけ書いて MessageTooLong を返す
	std::array<nox::uint8, 4> received = {};
	const network::IoResult result = network::ReceiveFrom(receiver.socket, received, nullptr);
	EXPECT_EQ(result.error, Error::MessageTooLong);
	EXPECT_EQ(result.bytes, 4);
	EXPECT_EQ(received[3], 4);
}

TEST_F(NetworkTest, Ipv6Loopback)
{
	ScopedSocket listener;
	if (network::CreateSocket(IpFamily::IPv6, Protocol::Tcp, listener.socket) != Error::None)
	{
		GTEST_SKIP() << "IPv6 のソケットを作れない環境";
	}
	ASSERT_EQ(network::SetIPv6Only(listener.socket, true), Error::None);

	const Error bind_error = network::Bind(listener.socket, EndPoint::IPv6Loopback(0));
	if (bind_error == Error::AddressNotAvailable)
	{
		GTEST_SKIP() << "IPv6 のループバックが無い環境";
	}
	ASSERT_EQ(bind_error, Error::None);

	EndPoint bound;
	ASSERT_EQ(network::GetLocalEndPoint(listener.socket, bound), Error::None);
	EXPECT_EQ(bound.family, IpFamily::IPv6);
	EXPECT_EQ(bound.address[15], 1);
	EXPECT_NE(bound.port, 0);
}
