#include "container.h"
#include <iostream>
#include <algorithm>
#include "../browser.h"

NFX_Container::NFX_Container(SDL_Renderer* renderer)
    : renderer(renderer), default_font_size(16), default_font_name("Roboto-Regular.ttf")
{
    // Load default fonts (similar to your existing renderer)
    fonts["default"] = TTF_OpenFont("Roboto-Regular.ttf", 16);
    fonts["h1"] = TTF_OpenFont("Roboto-Regular.ttf", 32);
    fonts["h2"] = TTF_OpenFont("Roboto-Regular.ttf", 24);
    fonts["h3"] = TTF_OpenFont("Roboto-Regular.ttf", 20);
}

NFX_Container::~NFX_Container()
{
    for (auto& font_pair : fonts) {
        if (font_pair.second) {
            TTF_CloseFont(font_pair.second);
        }
    }
}

litehtml::uint_ptr NFX_Container::create_font(const char* faceName, int size, int weight,
    litehtml::font_style italic, unsigned int decoration,
    litehtml::font_metrics* fm)
{
    // For now, use our default font with the requested size
    TTF_Font* font = TTF_OpenFont("Roboto-Regular.ttf", size);

    if (font && fm) {
        // Fill font metrics
        fm->height = TTF_FontHeight(font);
        fm->ascent = TTF_FontAscent(font);
        fm->descent = TTF_FontDescent(font);
        fm->x_height = fm->height / 2; // Approximation
    }

    return reinterpret_cast<litehtml::uint_ptr>(font);
}

void NFX_Container::delete_font(litehtml::uint_ptr hFont)
{
    if (hFont) {
        TTF_Font* font = reinterpret_cast<TTF_Font*>(hFont);
        TTF_CloseFont(font);
    }
}

int NFX_Container::text_width(const char* text, litehtml::uint_ptr hFont)
{
    if (!text || !hFont) return 0;

    TTF_Font* font = reinterpret_cast<TTF_Font*>(hFont);
    int width, height;

    if (TTF_SizeText(font, text, &width, &height) == 0) {
        return width;
    }

    return 0;
}

void NFX_Container::draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
    litehtml::web_color color, const litehtml::position& pos)
{
    if (!text || !hFont || !renderer) return;

    TTF_Font* font = reinterpret_cast<TTF_Font*>(hFont);

    SDL_Color sdl_color = {
        static_cast<Uint8>(color.red),
        static_cast<Uint8>(color.green),
        static_cast<Uint8>(color.blue),
        static_cast<Uint8>(color.alpha)
    };

    SDL_Surface* surface = TTF_RenderText_Solid(font, text, sdl_color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dst = { pos.x, pos.y, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
}

int NFX_Container::pt_to_px(int pt) const
{
    // Standard conversion: 1 point = 1.333 pixels at 96 DPI
    return static_cast<int>(pt * 1.333f);
}

int NFX_Container::get_default_font_size() const
{
    return default_font_size;
}

const char* NFX_Container::get_default_font_name() const
{
    return default_font_name.c_str();
}

void NFX_Container::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker)
{
    // Basic implementation - just draw bullet point for now
    if (marker.marker_type == litehtml::list_style_type_disc) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_Rect bullet = { marker.pos.x, marker.pos.y + 5, 4, 4 };
        SDL_RenderFillRect(renderer, &bullet);
    }
}

void NFX_Container::load_image(const char* src, const char* baseurl, bool redraw_on_ready)
{
    // TODO: Implement image loading
    // For now, just stub this out
}

void NFX_Container::get_image_size(const char* src, const char* baseurl, litehtml::size& sz)
{
    // TODO: Implement image size retrieval
    // For now, return default size
    sz.width = 100;
    sz.height = 100;
}

void NFX_Container::draw_background(litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bg)
{
    if (bg.empty()) return;

    // Draw the background color from the last background
    const auto& background = bg.back();
    if (background.color.alpha > 0) {
        SDL_SetRenderDrawColor(renderer,
            background.color.red,
            background.color.green,
            background.color.blue,
            background.color.alpha);

        SDL_Rect rect = {
            background.clip_box.x,
            background.clip_box.y,
            background.clip_box.width,
            background.clip_box.height
        };
        SDL_RenderFillRect(renderer, &rect);
    }
}

void NFX_Container::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders,
    const litehtml::position& draw_pos, bool root)
{
    // Simple border drawing - just draw rectangles for each border
    if (borders.top.width > 0) {
        SDL_SetRenderDrawColor(renderer,
            borders.top.color.red,
            borders.top.color.green,
            borders.top.color.blue,
            borders.top.color.alpha);
        SDL_Rect top = { draw_pos.x, draw_pos.y, draw_pos.width, borders.top.width };
        SDL_RenderFillRect(renderer, &top);
    }

    // Similar for other borders...
    if (borders.bottom.width > 0) {
        SDL_SetRenderDrawColor(renderer,
            borders.bottom.color.red,
            borders.bottom.color.green,
            borders.bottom.color.blue,
            borders.bottom.color.alpha);
        SDL_Rect bottom = { draw_pos.x, draw_pos.y + draw_pos.height - borders.bottom.width,
                           draw_pos.width, borders.bottom.width };
        SDL_RenderFillRect(renderer, &bottom);
    }

    if (borders.left.width > 0) {
        SDL_SetRenderDrawColor(renderer,
            borders.left.color.red,
            borders.left.color.green,
            borders.left.color.blue,
            borders.left.color.alpha);
        SDL_Rect left = { draw_pos.x, draw_pos.y, borders.left.width, draw_pos.height };
        SDL_RenderFillRect(renderer, &left);
    }

    if (borders.right.width > 0) {
        SDL_SetRenderDrawColor(renderer,
            borders.right.color.red,
            borders.right.color.green,
            borders.right.color.blue,
            borders.right.color.alpha);
        SDL_Rect right = { draw_pos.x + draw_pos.width - borders.right.width, draw_pos.y,
                          borders.right.width, draw_pos.height };
        SDL_RenderFillRect(renderer, &right);
    }
}

void NFX_Container::set_caption(const char* caption)
{
    // Set window title if available
    if (caption) {
        std::string title = "NetFX - ";
        title += caption;
        // We'd need window reference for this - for now just ignore
    }
}

void NFX_Container::set_base_url(const char* base_url)
{
    // Store base URL for relative link resolution
    // TODO: Implement base URL storage
}

void NFX_Container::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el)
{
    // Handle link elements (like <link rel="stylesheet">)
    // TODO: Implement CSS loading
}

void NFX_Container::on_anchor_click(const char* url, const litehtml::element::ptr& el)
{
    if (browser && url) {
        std::cout << "Container: Link clicked: " << url << std::endl;
        ((NFX_Browser*)browser)->on_anchor_click(std::string(url));
    }
}

void NFX_Container::set_cursor(const char* cursor)
{
    // TODO: Implement cursor changing
}

void NFX_Container::transform_text(litehtml::string& text, litehtml::text_transform tt)
{
    switch (tt) {
    case litehtml::text_transform_uppercase:
        std::transform(text.begin(), text.end(), text.begin(), ::toupper);
        break;
    case litehtml::text_transform_lowercase:
        std::transform(text.begin(), text.end(), text.begin(), ::tolower);
        break;
    case litehtml::text_transform_capitalize:
        // Capitalize first letter of each word
    {
        bool capitalize_next = true;
        for (char& c : text) {
            if (std::isspace(c)) {
                capitalize_next = true;
            }
            else if (capitalize_next) {
                c = std::toupper(c);
                capitalize_next = false;
            }
        }
    }
    break;
    default:
        break;
    }
}

void NFX_Container::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl)
{
    // TODO: Implement CSS importing
}

void NFX_Container::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius)
{
    // TODO: Implement clipping
}

void NFX_Container::del_clip()
{
    // TODO: Remove clipping
}

void NFX_Container::get_client_rect(litehtml::position& client) const
{
    // Return the rendering area size
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    client.x = 0;
    client.y = 0;
    client.width = w;
    client.height = h;
}

litehtml::element::ptr NFX_Container::create_element(const char* tag_name,
    const litehtml::string_map& attributes,
    const std::shared_ptr<litehtml::document>& doc)
{
    // Return nullptr to use default element creation
    return nullptr;
}

void NFX_Container::get_media_features(litehtml::media_features& media) const
{
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);

    media.type = litehtml::media_type_screen;
    media.width = w;
    media.height = h;
    media.device_width = w;
    media.device_height = h;
    media.color = 8;
    media.monochrome = 0;
    media.color_index = 256;
    media.resolution = 96; // 96 DPI
}

void NFX_Container::get_language(litehtml::string& language, litehtml::string& culture) const
{
    language = "en";
    culture = "US";
}

void NFX_Container::set_browser(void* browser_ref)
{
    this->browser = browser_ref;
}