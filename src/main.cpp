// Digital Workshop - Main Entry Point

#include "app/application.h"

#include <csignal>

namespace {
dw::Application* g_app = nullptr;

void signalHandler(int /*sig*/) {
    if (g_app) g_app->quit();
}
} // namespace

int main(int /*argc*/, char* /*argv*/[]) {
    dw::Application app;
    g_app = &app;

    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);

    if (!app.init()) {
        return 1;
    }

    int result = app.run();
    g_app = nullptr;
    return result;
}
