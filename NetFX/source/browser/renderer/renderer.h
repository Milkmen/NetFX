#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <map>
#include <vector>
#include <string>

typedef struct
{
	std::string tag;
	std::map<std::string, std::string> attributes;
	std::string inner;
	float x, y, w, h;
}
NFX_DrawObj_t;

#include "../../tree.hpp"

class NFX_Renderer
{
private:
	void renderSimpleText(TTF_Font* font, const char* text, int x, int y);
	void renderWrappedText(TTF_Font* font, const std::string& text, float& x, float& y, float maxWidth);
	TTF_Font* getFontForTag(const std::string& tag);

	void renderHtml(LCRS_Node<NFX_DrawObj_t>* node, float& x, float& y, float w, float maxWidth);

public:
	std::vector<NFX_DrawObj_t> rendered;
	LCRS_Tree<NFX_DrawObj_t> tree;

	SDL_Window* window;
	SDL_Renderer* renderer;
	std::map<std::string, TTF_Font*> fonts;

	NFX_Renderer() : window(nullptr) {}
	NFX_Renderer(SDL_Window* window);
	~NFX_Renderer();
	void render();
};