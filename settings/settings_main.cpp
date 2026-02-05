// Digital Workshop - Settings Application Entry Point

#include "settings_app.h"

int main(int /*argc*/, char* /*argv*/[]) {
    dw::SettingsApp app;

    if (!app.init()) {
        return 1;
    }

    return app.run();
}
