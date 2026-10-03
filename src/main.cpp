#include "game_manager.h"
#include "launch_options.h"
#include <cstdio>

int main(int argc, char** argv) {
    LaunchOptions opts = ParseLaunchOptions(std::vector<std::string>(argv + 1, argv + argc));
    if (!opts.Ok()) {
        std::fprintf(stderr, "Loi: %s\n%s", opts.error.c_str(), LaunchUsage());
        return 2;
    }
    GameManager game;
    game.Run(opts);
    return 0;
}
