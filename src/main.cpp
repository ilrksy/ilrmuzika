#include <clocale>
#include <cstring>
#include <iostream>
#include "app.h"

static void print_version() {
    std::cout <<
        "\033[1;36m"
        "  ██╗██╗     ██████╗ ███╗   ███╗██╗   ██╗███████╗██╗██╗  ██╗ █████╗ \n"
        "  ██║██║     ██╔══██╗████╗ ████║██║   ██║╚══███╔╝██║██║ ██╔╝██╔══██╗\n"
        "  ██║██║     ██████╔╝██╔████╔██║██║   ██║  ███╔╝ ██║█████╔╝ ███████║\n"
        "  ██║██║     ██╔══██╗██║╚██╔╝██║██║   ██║ ███╔╝  ██║██╔═██╗ ██╔══██║\n"
        "  ██║███████╗██║  ██║██║ ╚═╝ ██║╚██████╔╝███████╗██║██║  ██╗██║  ██║\n"
        "  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═╝╚═╝  ╚═╝\n"
        "\033[0m"
        "\033[1;32m  ✦ ilrmuzika  \033[0m"
        "| Created by Aidil | Apache 2.0 | C++17\n"
        "  \033[90mv1.0.0 — modern terminal music streaming & player\033[0m\n\n"
        "  APIs  : LRCLIB · MusicBrainz · Audius · YouTube Innertube\n"
        "  EQ    : Flat · Bass Boost · Vocal Boost · Treble Boost · Electronic\n"
        "  Themes: aurora-aidil · tokyo-night · catppuccin · cyberpunk · dracula\n"
    << std::endl;
}

static void print_help() {
    print_version();
    std::cout <<
        "Usage:\n"
        "  ilrmuzika                  Launch interactive TUI\n"
        "  ilrmuzika <file.mp3>       Play a specific file and launch TUI\n"
        "  ilrmuzika -v | --version   Print version\n"
        "  ilrmuzika -h | --help      Print this help\n\n"
        "Hotkeys (in-app):\n"
        "  /         Local search        /s: <q>   Online (YouTube) search\n"
        "  p/ENTER   Play/Pause          n/b       Next/Previous track\n"
        "  1/2       Vol up/down         x         Mute toggle\n"
        "  e         Cycle EQ preset     v         Cycle visualizer style\n"
        "  a/d       Queue add/remove    TAB       Switch List ↔ Queue focus\n"
        "  m         Cycle play mode     r         (repeat, via play mode)\n"
        "  w         Waveform style      s         Settings panel\n"
        "  t         Console/log         ?         Cheatsheet\n"
        "  f/c       Folder filter/clear l         Retry lyrics override\n"
        "  q         Quit\n"
    << std::endl;
}

int main(int argc, char* argv[]) {
    // Locale setup
    if (!std::setlocale(LC_ALL, "")) {
        std::setlocale(LC_ALL, "C.UTF-8");
    } else {
        const char* cur = std::setlocale(LC_CTYPE, nullptr);
        if (cur && std::string(cur) == "C") {
            std::setlocale(LC_ALL, "C.UTF-8");
        }
    }

    // CLI argument handling
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
            print_version();
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        }
    }

    muisc::App app;
    return app.run();
}
