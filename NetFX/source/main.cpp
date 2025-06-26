#include "browser/browser.h"

#include <SDL.h>
#include <SDL_ttf.h>

#define MAX_INPUT 256

bool searchBarActive = false;
std::string searchText = "";

int main(int argc, char* argv[]) 
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
    {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) 
    {
        printf("TTF init failed: %s\n", TTF_GetError());
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("NetFX",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600, SDL_WINDOW_SHOWN);

    if (!window) 
    {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    std::string urlText = "https://pornhub.com/";
    NFX_Url url = NFX_Url(urlText);
    NFX_Browser* browser = new NFX_Browser(window);
    browser->load(url);

    // Main loop
    SDL_Event e;
    int quit = 0;

    while (!quit) 
    {
        while (SDL_PollEvent(&e)) 
        {
            if (e.type == SDL_QUIT) 
            {
                quit = 1;
            }

            if (e.type == SDL_KEYDOWN) 
            {
                SDL_Keymod mod = SDL_GetModState();

                // Toggle search bar with Alt key (pressed down)
                if (e.key.keysym.sym == SDLK_LALT) 
                {
                    searchBarActive = !searchBarActive;

                    if (searchBarActive) 
                    {
                        SDL_StartTextInput();
                        searchText.clear();
                    }
                    else 
                    {
                        SDL_StopTextInput();
                    }
                }

                if (searchBarActive && e.key.keysym.sym == SDLK_BACKSPACE) 
                {
                    searchText = searchText.substr(0, searchText.length() - 1);
                }

                if (searchBarActive && e.key.keysym.sym == SDLK_RETURN) 
                {
                    url = NFX_Url(searchText);
                    browser->load(url);
                    searchBarActive = false;
                    SDL_StopTextInput();
                }
            }

            // Capture text input
            if (e.type == SDL_TEXTINPUT && searchBarActive) {
                searchText.append(e.text.text);
            }
        }

        browser->render();

        SDL_Delay(16); // ~60 FPS
    }

    delete browser;

    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}