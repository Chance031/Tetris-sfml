#include "Game.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    int RunGame()
    {
        // Create the game object and start the main loop.
        Game game;
        game.Run();
        return 0;
    }
}

#ifdef NDEBUG
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    return RunGame();
}
#else
int main()
{
    // Create the game object and start the main loop.
    return RunGame();
}
#endif