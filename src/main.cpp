// Digital Workshop - Main Entry Point

#include "app/application.h"

int main(int /*argc*/, char* /*argv*/[]) {
    dw::Application app;

    if (!app.init()) {
        return 1;
    }

    return app.run();
}
