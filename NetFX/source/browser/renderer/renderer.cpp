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

void NFX_Renderer::renderSimpleText(TTF_Font* font, const char* text, int x, int y)
{
    if (!this->renderer || !text || !font) return;
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

void NFX_Renderer::renderWrappedText(TTF_Font* font, const std::string& text, float& x, float& y, float maxWidth) 
{
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

        renderSimpleText(font, word.c_str(), x, y);
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

    if (el.tag == "#text") 
    {
        if (!el.inner.empty()) 
        {
            // Use appropriate font for parent element
            TTF_Font* font = fonts["default"];
            int textW, textH;

            if (node->parent)
            {
                font = this->getFontForTag(node->parent->value.tag);
            }

            if (TTF_SizeText(font, el.inner.c_str(), &textW, &textH) == 0) 
            {
                // Check if we need to wrap to next line
                if (x + textW > maxWidth && x > 10) 
                {
                    x = 10; // Reset to left margin
                    y += textH + 2; // Move to next line
                }

                this->renderSimpleText(font, el.inner.c_str(), (int)x, (int)y);
                x += textW + 5; // Add some space after text
            }
        }
    }
    else 
    {
        // Handle different HTML elements
        bool isBlock = isBlockElement(el.tag);

        if (isBlock) 
        {
            if (x > 10) 
            {
                x = 10;
                y += 5; // Add some spacing
            }
        }

        // Add element-specific spacing
        if (el.tag == "h1" || el.tag == "h2" || el.tag == "h3") {
            y += 15; // Extra spacing for headers
        }
        else if (el.tag == "p") {
            y += 10; // Paragraph spacing
        }
        else if (el.tag == "br") {
            // Line break
            x = 10;
            y += 20;
            return; // Don't process children for <br>
        }

        // Render all children
        LCRS_Node<NFX_DrawObj_t>* child = node->child;
        while (child) {
            renderHtml(child, x, y, w, maxWidth);
            child = child->sibling;
        }

        // Add spacing after block elements
        if (isBlock) {
            x = 10;
            y += 5;
        }
    }
}

void NFX_Renderer::render()
{
    extern bool searchBarActive;
    extern std::string searchText;

    if (!this->window || !this->renderer) return;
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
    SDL_RenderClear(this->renderer);

    // Fix: Use proper parameters with reference variables
    float x = 10.0f;  // Start with some margin
    float y = 10.0f;  // Start with some margin
    float w = 780.0f; // Width minus margins
    float maxWidth = 780.0f; // Max width for text wrapping

    this->renderHtml(this->tree.getRoot(), x, y, w, maxWidth);

    if (searchBarActive) {
        this->renderSimpleText(this->fonts["h1"], searchText.c_str(), 16, 16);
    }

    SDL_RenderPresent(this->renderer);
}