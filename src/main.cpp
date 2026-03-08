// Digital Workshop - Main Entry Point

#include "app/application.h"
#include "core/utils/log.h"

#include <csignal>
#include <cstring>

namespace {
dw::Application* g_app = nullptr;

void signalHandler(int /*sig*/) {
    if (g_app) g_app->quit();
}
} // namespace

int main(int argc, char* argv[]) {
    // Enable console log output only with --verbose / -v / -V
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--verbose") == 0 ||
            std::strcmp(argv[i], "-v") == 0 ||
            std::strcmp(argv[i], "-V") == 0) {
            dw::log::setConsoleOutput(true);
            break;
        }
    }

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
