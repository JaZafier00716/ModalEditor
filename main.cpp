#include <filesystem>
#include <iostream>
#include <print>
#include <string_view>

#include "include/Editor.h"

#ifndef MOTION_CONFIG_DIR
#define MOTION_CONFIG_DIR "."
#endif

// TODO:
/*
 * Main parameters:
 *  - use filename inputted as a message on the first line centered
 *      - if no filename specified use 'new file' as a message
 *  - enable --<setting> for additional configuration (aside from config file)
 *  - config:
 *      - tab spaces
 *      - line numbers
 *      - allow arrows for movement
*/

/**
 * @brief Program entry point.
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument values.
 * @return Process exit status code.
 */
int main (const int argc, char* argv[]) {
    bool use_default_config = false;
    std::string file_name;

    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument == "--default") {
            use_default_config = true;
        }
        else if (file_name.empty()) {
            file_name = argv[i];
        }
    }

    const auto config_directory = std::filesystem::path{MOTION_CONFIG_DIR};
    const auto config_path = config_directory / (use_default_config ? "motions.default.conf" : "motions.conf");

    std::string config_error;
    if (!loadNormalModeConfig(config_path.string(), config_error)) {
        std::cerr << "Warning: " << config_error << '\n';
    }

    Editor editor{file_name};
    editor.run();

    return 0;
}
