#include "tcp_socket.h"

#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "../utils/log.h"

namespace dw {

// ── POSIX implementation ──────────────────────────────────────────────

#ifndef _WIN32

TcpSocket::~TcpSocket() {
    close();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : m_fd(other.m_fd), m_device(std::move(other.m_device)),
      m_readBuffer(std::move(other.m_readBuffer)),
      m_connectionState(other.m_connectionState) {
    other.m_fd = -1;
    other.m_connectionState = ConnectionState::Closed;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        close();
        m_fd = other.m_fd;
        m_device = std::move(other.m_device);
        m_readBuffer = std::move(other.m_readBuffer);
        m_connectionState = other.m_connectionState;
        other.m_fd = -1;
        other.m_connectionState = ConnectionState::Closed;
    }
    return *this;
}

bool TcpSocket::connect(const std::string& host, int port, int timeoutMs) {
    close();

    // Resolve host
    struct addrinfo hints {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    std::string portStr = std::to_string(port);
    struct addrinfo* result = nullptr;
    int rv = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (rv != 0) {
        log::errorf("TCP", "Failed to resolve %s: %s", host.c_str(), gai_strerror(rv));
        return false;
    }

    // Create socket
    m_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_fd < 0) {
        log::errorf("TCP", "socket() failed: %s", strerror(errno));
        freeaddrinfo(result);
        return false;
    }

    // Set non-blocking for connect timeout
    int flags = fcntl(m_fd, F_GETFL, 0);
    fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);

    // Initiate connection
    rv = ::connect(m_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);

    if (rv < 0 && errno != EINPROGRESS) {
        log::errorf("TCP", "connect() to %s:%d failed: %s", host.c_str(), port, strerror(errno));
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    if (rv < 0) {
        // Wait for connection with timeout
        struct pollfd pfd {};
        pfd.fd = m_fd;
        pfd.events = POLLOUT;

        int pollRv = poll(&pfd, 1, timeoutMs);
        if (pollRv <= 0) {
            log::errorf("TCP", "Connect to %s:%d timed out", host.c_str(), port);
            ::close(m_fd);
            m_fd = -1;
            return false;
        }

        // Check for connection errors
        int sockErr = 0;
        socklen_t errLen = sizeof(sockErr);
        getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &sockErr, &errLen);
        if (sockErr != 0) {
            log::errorf("TCP", "Connect to %s:%d failed: %s", host.c_str(), port, strerror(sockErr));
            ::close(m_fd);
            m_fd = -1;
            return false;
        }
    }

    // Enable TCP_NODELAY (disable Nagle's algorithm for real-time CNC commands)
    int nodelay = 1;
    setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    m_device = host + ":" + std::to_string(port);
    m_readBuffer.clear();
    m_connectionState = ConnectionState::Connected;
    log::infof("TCP", "Connected to %s", m_device.c_str());
    return true;
}

void TcpSocket::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
        log::infof("TCP", "Closed %s", m_device.c_str());
    }
    m_readBuffer.clear();
    m_connectionState = ConnectionState::Closed;
}

bool TcpSocket::write(const std::string& data) {
    if (m_fd < 0)
        return false;

    size_t totalWritten = 0;
    while (totalWritten < data.size()) {
        ssize_t written = ::write(m_fd, data.c_str() + totalWritten,
                                  data.size() - totalWritten);
        if (written < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                // Wait for socket to become writable
                struct pollfd pfd {};
                pfd.fd = m_fd;
                pfd.events = POLLOUT;
                int rv = poll(&pfd, 1, 1000);
                if (rv <= 0) {
                    log::error("TCP", "Write timed out");
                    return false;
                }
                continue;
            }
            if (errno == EPIPE || errno == ECONNRESET)
                m_connectionState = ConnectionState::Disconnected;
            log::errorf("TCP", "Write failed: %s", strerror(errno));
            return false;
        }
        totalWritten += static_cast<size_t>(written);
    }
    return true;
}

bool TcpSocket::writeByte(u8 byte) {
    if (m_fd < 0)
        return false;

    ssize_t written = ::write(m_fd, &byte, 1);
    if (written != 1) {
        if (errno == EPIPE || errno == ECONNRESET)
            m_connectionState = ConnectionState::Disconnected;
        return false;
    }
    return true;
}

std::optional<std::string> TcpSocket::readLine(int timeoutMs) {
    if (m_fd < 0)
        return std::nullopt;

    // Check if we already have a complete line in the buffer
    auto nlPos = m_readBuffer.find('\n');
    if (nlPos != std::string::npos) {
        std::string line = m_readBuffer.substr(0, nlPos);
        m_readBuffer.erase(0, nlPos + 1);
        // Strip trailing \r
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        return line;
    }

    // Poll for data with monotonic clock for accurate timeout tracking
    struct pollfd pfd {};
    pfd.fd = m_fd;
    pfd.events = POLLIN;

    auto startTime = std::chrono::steady_clock::now();
    int remaining = timeoutMs;

    while (remaining > 0) {
        int ret = poll(&pfd, 1, remaining);
        if (ret < 0) {
            if (errno == EINTR) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime).count();
                remaining = timeoutMs - static_cast<int>(elapsed);
                continue;
            }
            m_connectionState = ConnectionState::Error;
            return std::nullopt;
        }
        if (ret == 0)
            return std::nullopt; // Timeout

        // Check for disconnect/error
        if (pfd.revents & (POLLHUP | POLLERR)) {
            log::error("TCP", "Connection lost (POLLHUP/POLLERR)");
            m_connectionState = ConnectionState::Disconnected;
            return std::nullopt;
        }

        if (pfd.revents & POLLIN) {
            char buf[256];
            ssize_t n = ::read(m_fd, buf, sizeof(buf));
            if (n == 0) {
                // Clean disconnect (FIN received)
                log::info("TCP", "Connection closed by peer");
                m_connectionState = ConnectionState::Disconnected;
                return std::nullopt;
            }
            if (n < 0) {
                m_connectionState = ConnectionState::Disconnected;
                return std::nullopt;
            }

            m_readBuffer.append(buf, static_cast<size_t>(n));

            nlPos = m_readBuffer.find('\n');
            if (nlPos != std::string::npos) {
                std::string line = m_readBuffer.substr(0, nlPos);
                m_readBuffer.erase(0, nlPos + 1);
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                return line;
            }
        }

        // Recalculate remaining time
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        remaining = timeoutMs - static_cast<int>(elapsed);
    }

    return std::nullopt;
}

void TcpSocket::drain() {
    if (m_fd < 0)
        return;
    char buf[256];
    // Read and discard any pending data (non-blocking)
    while (::read(m_fd, buf, sizeof(buf)) > 0) {}
    m_readBuffer.clear();
}

// ── Windows stubs (TCP not yet implemented) ───────────────────────────

#else // _WIN32

TcpSocket::~TcpSocket() { close(); }
TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : m_fd(other.m_fd), m_device(std::move(other.m_device)),
      m_readBuffer(std::move(other.m_readBuffer)),
      m_connectionState(other.m_connectionState) {
    other.m_fd = -1;
    other.m_connectionState = ConnectionState::Closed;
}
TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        close();
        m_fd = other.m_fd;
        m_device = std::move(other.m_device);
        m_readBuffer = std::move(other.m_readBuffer);
        m_connectionState = other.m_connectionState;
        other.m_fd = -1;
        other.m_connectionState = ConnectionState::Closed;
    }
    return *this;
}

bool TcpSocket::connect(const std::string& host, int /*port*/, int /*timeoutMs*/) {
    log::errorf("TCP", "TCP socket not yet implemented on Windows: %s", host.c_str());
    return false;
}
void TcpSocket::close() { m_fd = -1; m_readBuffer.clear(); m_connectionState = ConnectionState::Closed; }
bool TcpSocket::write(const std::string& /*data*/) { return false; }
bool TcpSocket::writeByte(u8 /*byte*/) { return false; }
std::optional<std::string> TcpSocket::readLine(int /*timeoutMs*/) { return std::nullopt; }
void TcpSocket::drain() { m_readBuffer.clear(); }

#endif // _WIN32

} // namespace dw
