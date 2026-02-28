#pragma once

#include <optional>
#include <string>

#include "../types.h"

namespace dw {

// Transport-agnostic connection state
enum class ConnectionState {
    Closed,        // Not connected
    Connected,     // Open and healthy
    Disconnected,  // Peer gone (cable pulled, TCP FIN, etc.)
    Error          // Unrecoverable error
};

// Abstract byte-stream interface for CNC transports (serial, TCP, etc.)
class IByteStream {
  public:
    virtual ~IByteStream() = default;

    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual bool write(const std::string& data) = 0;
    virtual bool writeByte(u8 byte) = 0;
    virtual std::optional<std::string> readLine(int timeoutMs) = 0;
    virtual void drain() = 0;
    virtual const std::string& device() const = 0;
    virtual ConnectionState connectionState() const = 0;
};

} // namespace dw
