#pragma once

#include <iostream>
#include <thread>

namespace dw {
namespace threading {

// Store main thread ID at Application::init()
inline std::thread::id g_mainThreadId;

inline void initMainThread() {
    g_mainThreadId = std::this_thread::get_id();
}

inline bool isMainThread() {
    return std::this_thread::get_id() == g_mainThreadId;
}

} // namespace threading
} // namespace dw

// Debug-only assertion macro (disabled in release builds)
// Follows existing GL_CHECK pattern from src/render/gl_utils.h
#ifdef NDEBUG
    #define ASSERT_MAIN_THREAD() ((void)0)
#else
    #define ASSERT_MAIN_THREAD()                                                                   \
        do {                                                                                       \
            if (!dw::threading::isMainThread()) {                                                  \
                std::cerr << "THREADING VIOLATION: " << __FILE__ << ":" << __LINE__ << " - "       \
                          << __func__ << " must be called from main thread\n";                     \
                std::abort();                                                                      \
            }                                                                                      \
        } while (0)
#endif
