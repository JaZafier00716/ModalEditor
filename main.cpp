#include <print>

#include "include/Editor.h"

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
    const std::string file_name = (argc > 1) ? argv[1] : "";

    Editor editor{file_name};
    editor.run();

    return 0;
}
