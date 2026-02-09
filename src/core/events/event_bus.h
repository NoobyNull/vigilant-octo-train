#pragma once

#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "core/utils/log.h"
#include "core/utils/thread_utils.h"

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
        // Wrap handler in a shared_ptr to manage lifetime
        auto handlerFunc = std::make_shared<std::function<void(const EventType&)>>(
            std::forward<Handler>(handler));

        // Store a weak_ptr to the handler in the handlers map
        auto typeIndex = std::type_index(typeid(EventType));
        m_handlers[typeIndex].push_back(handlerFunc);

        // Return the shared_ptr (as shared_ptr<void>) so caller controls lifetime
        return handlerFunc;
    }

    // Publish an event to all subscribed handlers
    // Handlers are invoked synchronously in registration order
    // Exception in one handler does not prevent others from running
    template <typename EventType>
    void publish(const EventType& event) {
        ASSERT_MAIN_THREAD();

        auto typeIndex = std::type_index(typeid(EventType));
        auto it = m_handlers.find(typeIndex);
        if (it == m_handlers.end()) {
            return; // No subscribers
        }

        // Lock all handlers upfront to keep them alive during publish
        // This ensures handlers remain valid even if subscription is dropped during iteration
        std::vector<std::shared_ptr<std::function<void(const EventType&)>>> lockedHandlers;
        for (auto& weakHandler : it->second) {
            auto locked = weakHandler.lock();
            if (locked) {
                lockedHandlers.push_back(
                    std::static_pointer_cast<std::function<void(const EventType&)>>(locked));
            }
        }

        // Invoke all handlers (safe from iterator invalidation)
        for (auto& handler : lockedHandlers) {
            try {
                (*handler)(event);
            } catch (const std::exception& e) {
                dw::log::errorf("event_bus", "Handler exception: %s", e.what());
            } catch (...) {
                dw::log::error("event_bus", "Handler threw unknown exception");
            }
        }

        // Clean up expired weak_ptrs from the original list
        auto& originalList = it->second;
        originalList.erase(
            std::remove_if(originalList.begin(), originalList.end(),
                           [](const std::weak_ptr<void>& wp) { return wp.expired(); }),
            originalList.end());
    }

private:
    // Internal storage: type_index -> list of weak_ptr to handlers
    std::unordered_map<std::type_index, std::vector<std::weak_ptr<void>>> m_handlers;
};

} // namespace dw
