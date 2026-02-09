// Digital Workshop - EventBus Tests

#include <gtest/gtest.h>

#include <stdexcept>

#include "core/events/event_bus.h"
#include "core/events/event_types.h"

TEST(EventBus, SubscribeAndReceive_SingleSubscriber) {
    dw::EventBus bus;
    int callCount = 0;
    int64_t receivedId = 0;
    std::string receivedName;

    auto sub = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& event) {
            callCount++;
            receivedId = event.newModelId;
            receivedName = event.modelName;
        });

    dw::WorkspaceChanged event{42, "TestModel"};
    bus.publish(event);

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(receivedId, 42);
    EXPECT_EQ(receivedName, "TestModel");
}

TEST(EventBus, SubscribeAndReceive_MultipleSubscribers) {
    dw::EventBus bus;
    int callCount1 = 0;
    int callCount2 = 0;

    auto sub1 = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& /*event*/) {
            callCount1++;
        });

    auto sub2 = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& /*event*/) {
            callCount2++;
        });

    dw::WorkspaceChanged event{42, "TestModel"};
    bus.publish(event);

    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
}

TEST(EventBus, SubscribeAndReceive_DifferentEventTypes) {
    dw::EventBus bus;
    int workspaceCallCount = 0;
    int importCallCount = 0;

    auto sub1 = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& /*event*/) {
            workspaceCallCount++;
        });

    auto sub2 = bus.subscribe<dw::ImportCompleted>(
        [&](const dw::ImportCompleted& /*event*/) {
            importCallCount++;
        });

    dw::WorkspaceChanged wsEvent{42, "TestModel"};
    bus.publish(wsEvent);

    dw::ImportCompleted importEvent{100, "ImportedModel"};
    bus.publish(importEvent);

    EXPECT_EQ(workspaceCallCount, 1);
    EXPECT_EQ(importCallCount, 1);
}

TEST(EventBus, Publish_NoSubscribers_DoesNotCrash) {
    dw::EventBus bus;

    // Should not crash even with no subscribers
    dw::WorkspaceChanged event{42, "TestModel"};
    bus.publish(event);

    // If we reach here, test passes
    EXPECT_TRUE(true);
}

TEST(EventBus, WeakRefCleanup_ExpiredSubscriberRemoved) {
    dw::EventBus bus;
    int callCount = 0;

    {
        auto sub = bus.subscribe<dw::WorkspaceChanged>(
            [&](const dw::WorkspaceChanged& /*event*/) {
                callCount++;
            });

        dw::WorkspaceChanged event{42, "TestModel"};
        bus.publish(event);

        EXPECT_EQ(callCount, 1);

        // sub goes out of scope here
    }

    // Publish again - handler should NOT be called since SubscriptionId expired
    dw::WorkspaceChanged event2{43, "TestModel2"};
    bus.publish(event2);

    EXPECT_EQ(callCount, 1); // Still 1, not 2
}

TEST(EventBus, WeakRefCleanup_MixedAliveAndExpired) {
    dw::EventBus bus;
    int callCount1 = 0;
    int callCount2 = 0;

    auto sub1 = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& /*event*/) {
            callCount1++;
        });

    {
        auto sub2 = bus.subscribe<dw::WorkspaceChanged>(
            [&](const dw::WorkspaceChanged& /*event*/) {
                callCount2++;
            });

        dw::WorkspaceChanged event{42, "TestModel"};
        bus.publish(event);

        EXPECT_EQ(callCount1, 1);
        EXPECT_EQ(callCount2, 1);

        // sub2 expires here
    }

    // Publish again - only sub1 should fire
    dw::WorkspaceChanged event2{43, "TestModel2"};
    bus.publish(event2);

    EXPECT_EQ(callCount1, 2); // Called twice
    EXPECT_EQ(callCount2, 1); // Still 1, not 2
}

TEST(EventBus, ReentrancySafety_SubscribeDuringPublish) {
    dw::EventBus bus;
    int callCount = 0;
    dw::EventBus::SubscriptionId newSub;

    auto sub1 = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& /*event*/) {
            callCount++;
            // Subscribe a new handler during publish
            newSub = bus.subscribe<dw::WorkspaceChanged>(
                [&](const dw::WorkspaceChanged& /*event*/) {
                    callCount += 10;
                });
        });

    dw::WorkspaceChanged event{42, "TestModel"};
    bus.publish(event);

    // First handler was called
    EXPECT_GE(callCount, 1);

    // Publish again - the new subscription should definitely be included now
    dw::WorkspaceChanged event2{43, "TestModel2"};
    bus.publish(event2);

    // Both handlers should have fired (1 + 10 from first, 1 + 10 from second)
    EXPECT_GE(callCount, 11);
}

TEST(EventBus, ExceptionIsolation_HandlerThrowDoesNotBlockOthers) {
    dw::EventBus bus;
    int callCount1 = 0;
    int callCount2 = 0;

    auto sub1 = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& /*event*/) {
            callCount1++;
            throw std::runtime_error("Handler exception");
        });

    auto sub2 = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& /*event*/) {
            callCount2++;
        });

    dw::WorkspaceChanged event{42, "TestModel"};
    bus.publish(event);

    // First handler was called (and threw)
    EXPECT_EQ(callCount1, 1);
    // Second handler should still have been called
    EXPECT_EQ(callCount2, 1);
}

TEST(EventBus, EmptyEvent_WorksWithNoFields) {
    dw::EventBus bus;
    int callCount = 0;

    auto sub = bus.subscribe<dw::ConfigFileChanged>(
        [&](const dw::ConfigFileChanged& /*event*/) {
            callCount++;
        });

    dw::ConfigFileChanged event{};
    bus.publish(event);

    EXPECT_EQ(callCount, 1);
}

TEST(EventBus, SubscriptionId_KeepsHandlerAlive) {
    dw::EventBus bus;
    int callCount = 0;

    auto sub = bus.subscribe<dw::WorkspaceChanged>(
        [&](const dw::WorkspaceChanged& /*event*/) {
            callCount++;
        });

    // Publish multiple times - handler should be called each time
    for (int i = 0; i < 5; i++) {
        dw::WorkspaceChanged event{static_cast<int64_t>(i), "TestModel"};
        bus.publish(event);
    }

    EXPECT_EQ(callCount, 5);
}
