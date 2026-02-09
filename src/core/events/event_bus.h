#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace dw {

// Type-safe event bus for decoupled subsystem communication
// THREADING CONTRACT: Main thread only - no internal synchronization
class EventBus {
public:
    // Subscription token - caller holds this to keep handler alive
    using SubscriptionId = std::shared_ptr<void>;

    // Subscribe to events of type EventType
    // Returns a subscription token - handler remains active while token is alive
    template <typename EventType, typename Handler>
    SubscriptionId subscribe(Handler&& handler) {
        // STUB: Does nothing - tests will fail
        (void)handler; // Suppress unused warning
        return nullptr;
    }

    // Publish an event to all subscribed handlers
    // Handlers are invoked synchronously in registration order
    // Exception in one handler does not prevent others from running
    template <typename EventType>
    void publish(const EventType& event) {
        // STUB: Does nothing - tests will fail
        (void)event; // Suppress unused warning
    }

private:
    // Internal storage: type_index -> list of weak_ptr to handlers
    std::unordered_map<std::type_index, std::vector<std::weak_ptr<void>>> m_handlers;
};

} // namespace dw
