#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <map>
#include <vector>
#include <string>
#include <gumbo.h>

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
	void render_simple_text(const char* text, int x, int y);
	void renderHtml(LCRS_Node<NFX_DrawObj_t>* node, float x, float y, float w, float h);
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