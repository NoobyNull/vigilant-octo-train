// Tests for SerialPort — unit tests that don't require real hardware
#include <gtest/gtest.h>

#include "core/cnc/serial_port.h"

using namespace dw;

TEST(SerialPort, ListPortsReturnsVector) {
    auto ports = listSerialPorts();
    // Should return a vector (possibly empty if no devices attached)
    EXPECT_GE(ports.size(), 0u);
    // If ports exist, they should start with /dev/tty
    for (const auto& port : ports) {
        EXPECT_TRUE(port.find("/dev/tty") == 0);
    }
}

TEST(SerialPort, DefaultNotOpen) {
    SerialPort port;
    EXPECT_FALSE(port.isOpen());
}

TEST(SerialPort, BadDeviceReturnsFalse) {
    SerialPort port;
    EXPECT_FALSE(port.open("/dev/nonexistent_serial_device_xyz", 115200));
    EXPECT_FALSE(port.isOpen());
}

TEST(SerialPort, DoubleCloseSafe) {
    SerialPort port;
    // Close on never-opened port should not crash
    EXPECT_NO_THROW(port.close());
    EXPECT_NO_THROW(port.close());
}

TEST(SerialPort, WriteOnClosedPortFails) {
    SerialPort port;
    EXPECT_FALSE(port.write("G0 X0\n"));
    EXPECT_FALSE(port.writeByte(0x18));
}

TEST(SerialPort, ReadLineOnClosedPortReturnsNullopt) {
    SerialPort port;
    auto result = port.readLine(10);
    EXPECT_FALSE(result.has_value());
}

TEST(SerialPort, MoveConstruction) {
    SerialPort a;
    // Move an unopened port — should not crash
    SerialPort b(std::move(a));
    EXPECT_FALSE(b.isOpen());
}

TEST(SerialPort, MoveAssignment) {
    SerialPort a;
    SerialPort b;
    b = std::move(a);
    EXPECT_FALSE(b.isOpen());
}

TEST(SerialPort, DrainOnClosedPortSafe) {
    SerialPort port;
    EXPECT_NO_THROW(port.drain());
}
