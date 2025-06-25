#include "renderer.h"
#include <SDL_ttf.h>
#include <gumbo.h>
#include <string>

NFX_Renderer::NFX_Renderer(SDL_Window* window)
{
    this->window = window;
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!this->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        exit(-1);
        return;
    }

    this->fonts["default-nfx"] = TTF_OpenFont("Roboto-Regular.ttf", 16);
}

NFX_Renderer::~NFX_Renderer()
{
    for (auto& x : this->fonts)
    {
        TTF_CloseFont(x.second);
    }
    SDL_DestroyRenderer(this->renderer);
}

void NFX_Renderer::render_simple_text(const char* text, int x, int y)
{
    if (!this->renderer || !text) return;
    TTF_Font* font = this->fonts["default-nfx"];
    if (!font) return;
    SDL_Color black = { 0, 0, 0, 255 };
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, black);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
    if (texture) 
    {
        SDL_Rect dst = { x, y, surface->w, surface->h };
        SDL_RenderCopy(this->renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

void NFX_Renderer::renderHtml(LCRS_Node<NFX_DrawObj_t>* node, float x, float y, float w, float h) 
{
    if (!node) return;

    unsigned char r = ((float)rand() / RAND_MAX) * 255;
    unsigned char g = ((float)rand() / RAND_MAX) * 255;
    unsigned char b = ((float)rand() / RAND_MAX) * 255;

    float height = 0;
    

    const NFX_DrawObj_t& el = node->value;
    if (el.tag == "#text") 
    { 
        height += 20;
        this->render_simple_text(el.inner.c_str(), x, y);
    }
    else 
    {
        renderHtml(node->child, x + 20, y + height, w - 40, 20); // Render children
    }

    renderHtml(node->sibling, x, y + height, w, 20);

    SDL_Rect rect = { x, y, w, height }; // x, y, width, height
    SDL_SetRenderDrawColor(this->renderer, r, g, b, 255);
    SDL_RenderDrawRect(this->renderer, &rect); // Filled rectangl   
}

void NFX_Renderer::render()
{
    if (!this->window || !this->renderer) return;
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
    SDL_RenderClear(this->renderer);

    this->renderHtml(this->tree.getRoot(), 0, 0, 800, 600);

    SDL_RenderPresent(this->renderer);
}