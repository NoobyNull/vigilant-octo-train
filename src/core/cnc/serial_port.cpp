#include "serial_port.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include "../utils/log.h"

namespace dw {

namespace {

speed_t toBaudConstant(int baudRate) {
    switch (baudRate) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 921600: return B921600;
    default: return B115200;
    }
}

} // namespace

SerialPort::~SerialPort() {
    close();
}

SerialPort::SerialPort(SerialPort&& other) noexcept
    : m_fd(other.m_fd), m_device(std::move(other.m_device)),
      m_readBuffer(std::move(other.m_readBuffer)) {
    other.m_fd = -1;
}

SerialPort& SerialPort::operator=(SerialPort&& other) noexcept {
    if (this != &other) {
        close();
        m_fd = other.m_fd;
        m_device = std::move(other.m_device);
        m_readBuffer = std::move(other.m_readBuffer);
        other.m_fd = -1;
    }
    return *this;
}

bool SerialPort::open(const std::string& device, int baudRate) {
    close();

    m_fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) {
        log::errorf("Serial", "Failed to open %s: %s", device.c_str(), strerror(errno));
        return false;
    }

    // Configure 8N1, no flow control
    struct termios tty {};
    if (tcgetattr(m_fd, &tty) != 0) {
        log::errorf("Serial", "tcgetattr failed: %s", strerror(errno));
        close();
        return false;
    }

    speed_t baud = toBaudConstant(baudRate);
    cfsetispeed(&tty, baud);
    cfsetospeed(&tty, baud);

    // Raw mode
    cfmakeraw(&tty);

    // 8N1
    tty.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
    tty.c_cflag |= CS8;

    // Enable receiver, ignore modem status lines
    tty.c_cflag |= (CLOCAL | CREAD);

    // No hardware flow control
    tty.c_cflag &= ~CRTSCTS;

    // No software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Non-blocking reads (poll-based)
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        log::errorf("Serial", "tcsetattr failed: %s", strerror(errno));
        close();
        return false;
    }

    // Flush any stale data
    tcflush(m_fd, TCIOFLUSH);

    m_device = device;
    m_readBuffer.clear();
    log::infof("Serial", "Opened %s at %d baud", device.c_str(), baudRate);
    return true;
}

void SerialPort::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
        log::infof("Serial", "Closed %s", m_device.c_str());
    }
    m_readBuffer.clear();
}

bool SerialPort::write(const std::string& data) {
    if (m_fd < 0)
        return false;

    ssize_t written = ::write(m_fd, data.c_str(), data.size());
    if (written < 0) {
        log::errorf("Serial", "Write failed: %s", strerror(errno));
        return false;
    }
    return static_cast<size_t>(written) == data.size();
}

bool SerialPort::writeByte(u8 byte) {
    if (m_fd < 0)
        return false;

    ssize_t written = ::write(m_fd, &byte, 1);
    return written == 1;
}

std::optional<std::string> SerialPort::readLine(int timeoutMs) {
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

    // Poll for data
    struct pollfd pfd {};
    pfd.fd = m_fd;
    pfd.events = POLLIN;

    int remaining = timeoutMs;
    while (remaining > 0) {
        int ret = poll(&pfd, 1, remaining);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            return std::nullopt;
        }
        if (ret == 0)
            return std::nullopt; // Timeout

        if (pfd.revents & POLLIN) {
            char buf[256];
            ssize_t n = ::read(m_fd, buf, sizeof(buf));
            if (n <= 0)
                return std::nullopt;

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

        // Approximate remaining time (poll doesn't tell us how long it waited)
        remaining -= 10;
    }

    return std::nullopt;
}

void SerialPort::drain() {
    if (m_fd < 0)
        return;
    tcdrain(m_fd);
    tcflush(m_fd, TCIFLUSH);
    m_readBuffer.clear();
}

std::vector<std::string> listSerialPorts() {
    std::vector<std::string> ports;

    namespace fs = std::filesystem;
    const std::string devPath = "/dev";

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(devPath, ec)) {
        if (!entry.is_character_file(ec))
            continue;
        std::string name = entry.path().filename().string();
        if (name.find("ttyUSB") == 0 || name.find("ttyACM") == 0) {
            ports.push_back(entry.path().string());
        }
    }

    std::sort(ports.begin(), ports.end());
    return ports;
}

} // namespace dw
