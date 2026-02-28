#include <gtest/gtest.h>

#include "core/cnc/tcp_socket.h"

using namespace dw;

TEST(TcpSocket, DefaultNotOpen) {
    TcpSocket sock;
    EXPECT_FALSE(sock.isOpen());
}

TEST(TcpSocket, DefaultConnectionState) {
    TcpSocket sock;
    EXPECT_EQ(sock.connectionState(), ConnectionState::Closed);
}

TEST(TcpSocket, BadHostReturnsFalse) {
    TcpSocket sock;
    EXPECT_FALSE(sock.connect("invalid.host.that.does.not.exist.example.com", 9999, 500));
    EXPECT_FALSE(sock.isOpen());
    EXPECT_EQ(sock.connectionState(), ConnectionState::Closed);
}

TEST(TcpSocket, BadPortReturnsFalse) {
    TcpSocket sock;
    // Port 1 is unlikely to have anything listening
    EXPECT_FALSE(sock.connect("127.0.0.1", 1, 500));
    EXPECT_FALSE(sock.isOpen());
}

TEST(TcpSocket, DoubleCloseSafe) {
    TcpSocket sock;
    sock.close();
    sock.close();
    EXPECT_FALSE(sock.isOpen());
}

TEST(TcpSocket, WriteOnClosedFails) {
    TcpSocket sock;
    EXPECT_FALSE(sock.write("test"));
    EXPECT_FALSE(sock.writeByte(0x18));
}

TEST(TcpSocket, ReadLineOnClosedReturnsNullopt) {
    TcpSocket sock;
    EXPECT_EQ(sock.readLine(10), std::nullopt);
}

TEST(TcpSocket, MoveConstruction) {
    TcpSocket sock;
    TcpSocket moved(std::move(sock));
    EXPECT_FALSE(moved.isOpen());
    EXPECT_EQ(moved.connectionState(), ConnectionState::Closed);
}

TEST(TcpSocket, MoveAssignment) {
    TcpSocket sock;
    TcpSocket moved;
    moved = std::move(sock);
    EXPECT_FALSE(moved.isOpen());
    EXPECT_EQ(moved.connectionState(), ConnectionState::Closed);
}

TEST(TcpSocket, DrainOnClosedSafe) {
    TcpSocket sock;
    sock.drain(); // Should not crash
    EXPECT_FALSE(sock.isOpen());
}

TEST(TcpSocket, DeviceEmptyWhenClosed) {
    TcpSocket sock;
    EXPECT_TRUE(sock.device().empty());
}
