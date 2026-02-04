#pragma once

#include "dialog.h"

#include <functional>

namespace dw {

// Message dialog types
enum class MessageType { Info, Warning, Error, Question };

// Simple message dialog
class MessageDialog : public Dialog {
public:
    MessageDialog();
    ~MessageDialog() override = default;

    void render() override;

    // Show a message
    void show(const std::string& title, const std::string& message,
              MessageType type = MessageType::Info,
              std::function<void(DialogResult)> callback = nullptr);

    // Quick show methods
    static void info(const std::string& title, const std::string& message);
    static void warning(const std::string& title, const std::string& message);
    static void error(const std::string& title, const std::string& message);

private:
    std::string m_message;
    MessageType m_type = MessageType::Info;
    std::function<void(DialogResult)> m_callback;

    // Singleton for static methods
    static MessageDialog& instance();
};

// Confirmation dialog
class ConfirmDialog : public Dialog {
public:
    ConfirmDialog();
    ~ConfirmDialog() override = default;

    void render() override;

    // Show confirmation
    void show(const std::string& title, const std::string& message,
              std::function<void(bool)> callback);

private:
    std::string m_message;
    std::function<void(bool)> m_callback;
};

}  // namespace dw
