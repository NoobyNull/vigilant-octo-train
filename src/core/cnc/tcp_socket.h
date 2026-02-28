#pragma once

#include "byte_stream.h"

#include <string>

namespace dw {

// Raw TCP socket transport for network-attached CNC controllers.
// Plain bidirectional byte stream (telnet-style) -- no framing, no WebSocket.
class TcpSocket : public IByteStream {
  public:
    TcpSocket() = default;
    ~TcpSocket() override;

    // Non-copyable, movable
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    // Connect to host:port (blocking connect with timeout)
    bool connect(const std::string& host, int port, int timeoutMs = 3000);

    // IByteStream interface
    void close() override;
    bool isOpen() const override { return m_fd >= 0; }
    bool write(const std::string& data) override;
    bool writeByte(u8 byte) override;
    std::optional<std::string> readLine(int timeoutMs) override;
    void drain() override;
    const std::string& device() const override { return m_device; }
    ConnectionState connectionState() const override { return m_connectionState; }

  private:
    int m_fd = -1;
    std::string m_device;       // "host:port" for display
    std::string m_readBuffer;   // Accumulates partial reads
    ConnectionState m_connectionState = ConnectionState::Closed;
};

} // namespace dw
