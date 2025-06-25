#include "renderer.h"
#include <SDL_ttf.h>
#include <gumbo.h>
#include <string>
#include <sstream>
#include <set>

NFX_Renderer::NFX_Renderer(SDL_Window* window)
{
    this->window = window;
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!this->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        exit(-1);
        return;
    }

    this->fonts["default"] = TTF_OpenFont("Roboto-Regular.ttf", 16);
    this->fonts["h1"] = TTF_OpenFont("Roboto-Regular.ttf", 32);
    this->fonts["h2"] = TTF_OpenFont("Roboto-Regular.ttf", 24);
    this->fonts["h3"] = TTF_OpenFont("Roboto-Regular.ttf", 20);
}

NFX_Renderer::~NFX_Renderer()
{
    for (auto& x : this->fonts)
    {
        TTF_CloseFont(x.second);
    }
    SDL_DestroyRenderer(this->renderer);
}

void NFX_Renderer::renderSimpleText(const char* text, int x, int y)
{
    if (!this->renderer || !text) return;
    TTF_Font* font = this->fonts["default"];
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

void NFX_Renderer::renderWrappedText(const std::string& text, float& x, float& y, float maxWidth) {
    TTF_Font* font = fonts["default"];
    if (!font) return;

    std::istringstream words(text);
    std::string word;

    while (words >> word) {
        int wordW, wordH;
        TTF_SizeText(font, word.c_str(), &wordW, &wordH);

        // Check if word fits on current line
        if (x + wordW > maxWidth && x > 0) {
            x = 0;
            y += wordH + 2; // Line height with small gap
        }

        renderSimpleText(word.c_str(), x, y);
        x += wordW + 5; // Add space between words
    }
}

bool isBlockElement(const std::string& tag) {
    static const std::set<std::string> blockTags = {
        "div", "p", "h1", "h2", "h3", "h4", "h5", "h6",
        "ul", "ol", "li", "blockquote", "pre", "hr", "br"
    };
    return blockTags.find(tag) != blockTags.end();
}


TTF_Font* NFX_Renderer::getFontForTag(const std::string& tag) {
    if (fonts.find(tag) != fonts.end()) {
        return fonts[tag];
    }
    return fonts["default"];
}

void NFX_Renderer::renderHtml(LCRS_Node<NFX_DrawObj_t>* node, float& x, float& y, float w, float maxWidth)
{
    if (!node) return;

    const NFX_DrawObj_t& el = node->value;

    if (el.tag == "#text") {
        if (!el.inner.empty()) {
            this->renderSimpleText(el.inner.c_str(), x, y);
            // Update position based on text size
            TTF_Font* font = fonts["default"];
            if (font) {
                int textW, textH;
                TTF_SizeText(font, el.inner.c_str(), &textW, &textH);
                x += textW;
                if (x > maxWidth) { // Line wrap
                    x = 0;
                    y += textH;
                }
            }
        }
    }
    else {
        // Handle block elements
        if (isBlockElement(el.tag)) {
            x = 0; // Start new line
            y += 20; // Add vertical spacing
        }

        // Add margins for specific elements
        float marginLeft = 0, marginTop = 0;
        if (el.tag == "h1" || el.tag == "h2" || el.tag == "h3") {
            marginTop = 10;
        }
        else if (el.tag == "p") {
            marginTop = 5;
        }

        y += marginTop;

        // Render children
        LCRS_Node<NFX_DrawObj_t>* child = node->child;
        while (child) {
            renderHtml(child, x, y, w, maxWidth);
            child = child->sibling;
        }

        // Handle block closing
        if (isBlockElement(el.tag)) {
            x = 0;
            y += 5; // Add spacing after block
        }
    }
}

void NFX_Renderer::render()
{
    if (!this->window || !this->renderer) return;
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
    SDL_RenderClear(this->renderer);

    this->renderHtml(this->tree.getRoot(), 0, 0, 800, 600);

    SDL_RenderPresent(this->renderer);
}