#include "container.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
#include <httplib.hpp>
#include <SDL_image.h>
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

    // Clean up loaded images
    for (auto& img_pair : loaded_images) {
        if (img_pair.second.texture) {
            SDL_DestroyTexture(img_pair.second.texture);
        }
    }
}

std::string NFX_Container::resolve_url(const std::string& src, const std::string& base_url)
{
    if (src.empty()) return "";

    // If it's already a full URL, return as-is
    if (src.find("://") != std::string::npos) {
        return src;
    }

    // If it starts with //, it's protocol-relative
    if (src.substr(0, 2) == "//") {
        // Determine protocol from base_url
        if (base_url.find("https://") == 0) {
            return "https:" + src;
        }
        else {
            return "http:" + src;
        }
    }

    // If it starts with /, it's absolute path
    if (src[0] == '/') {
        // Find the protocol and domain from base_url
        size_t proto_end = base_url.find("://");
        if (proto_end == std::string::npos) return src;

        size_t domain_end = base_url.find('/', proto_end + 3);
        if (domain_end == std::string::npos) {
            return base_url + src;
        }
        else {
            return base_url.substr(0, domain_end) + src;
        }
    }

    // It's a relative URL
    std::string resolved = base_url;
    if (!resolved.empty() && resolved.back() != '/') {
        resolved += "/";
    }
    resolved += src;

    return resolved;
}

void NFX_Container::load_image_async(const std::string& url, const std::string& src)
{
    std::thread([this, url, src]() {
        try {
            // Parse URL
            std::string remaining = url;
            std::string protocol, hostname, path;
            int port = 80;

            // Extract protocol
            size_t proto_pos = remaining.find("://");
            if (proto_pos != std::string::npos) {
                protocol = remaining.substr(0, proto_pos);
                remaining = remaining.substr(proto_pos + 3);

                if (protocol == "https") {
                    port = 443;
                }
            }
            else {
                protocol = "http";
            }

            // Extract hostname and path
            size_t path_pos = remaining.find('/');
            if (path_pos != std::string::npos) {
                hostname = remaining.substr(0, path_pos);
                path = remaining.substr(path_pos);
            }
            else {
                hostname = remaining;
                path = "/";
            }

            // Check for port in hostname
            size_t port_pos = hostname.find(':');
            if (port_pos != std::string::npos) {
                std::string port_str = hostname.substr(port_pos + 1);
                hostname = hostname.substr(0, port_pos);
                try {
                    port = std::stoi(port_str);
                }
                catch (...) {
                    // Keep default port
                }
            }

            // Download image
            httplib::Result res;
            httplib::Headers headers = {
                {"User-Agent", "NetFX Browser/1.0"},
                {"Accept", "image/*,*/*;q=0.8"},
                {"Connection", "close"}
            };

            if (protocol == "https") {
                httplib::SSLClient ssl_cli(hostname, port);
                ssl_cli.enable_server_certificate_verification(false);
                ssl_cli.set_connection_timeout(10, 0);
                ssl_cli.set_read_timeout(15, 0);
                ssl_cli.set_write_timeout(10, 0);
                res = ssl_cli.Get(path, headers);
            }
            else {
                httplib::Client http_cli(hostname, port);
                http_cli.set_connection_timeout(10, 0);
                http_cli.set_read_timeout(15, 0);
                http_cli.set_write_timeout(10, 0);
                res = http_cli.Get(path, headers);
            }

            if (res && res->status == 200) {
                // Create SDL texture from image data
                SDL_RWops* rw = SDL_RWFromConstMem(res->body.data(), res->body.size());
                if (rw) {
                    SDL_Surface* surface = IMG_Load_RW(rw, 1); // This frees the RWops
                    if (surface) {
                        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                        if (texture) {
                            // Store the loaded image
                            {
                                std::lock_guard<std::mutex> lock(images_mutex);
                                LoadedImage& img = loaded_images[src];
                                img.texture = texture;
                                img.width = surface->w;
                                img.height = surface->h;
                                img.loaded = true;

                                std::cout << "Image loaded: " << src << " (" << img.width << "x" << img.height << ")" << std::endl;
                            }
                        }
                        SDL_FreeSurface(surface);
                    }
                }
            }
            else {
                std::cout << "Failed to load image: " << url;
                if (res) {
                    std::cout << " (HTTP " << res->status << ")";
                }
                std::cout << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cout << "Exception loading image " << url << ": " << e.what() << std::endl;
        }
        }).detach();
}

void NFX_Container::load_image(const char* src, const char* baseurl, bool redraw_on_ready)
{
    if (!src) return;

    std::string src_str = src;
    std::string base_str = baseurl ? baseurl : current_base_url;

    // Check if already loaded or loading
    {
        std::lock_guard<std::mutex> lock(images_mutex);
        auto it = loaded_images.find(src_str);
        if (it != loaded_images.end()) {
            return; // Already loaded or loading
        }

        // Mark as loading
        loaded_images[src_str] = LoadedImage();
    }

    std::string full_url = resolve_url(src_str, base_str);
    std::cout << "Loading image: " << src_str << " -> " << full_url << std::endl;

    load_image_async(full_url, src_str);
}

void NFX_Container::get_image_size(const char* src, const char* baseurl, litehtml::size& sz)
{
    if (!src) {
        sz.width = 0;
        sz.height = 0;
        return;
    }

    std::string src_str = src;

    std::lock_guard<std::mutex> lock(images_mutex);
    auto it = loaded_images.find(src_str);
    if (it != loaded_images.end() && it->second.loaded) {
        sz.width = it->second.width;
        sz.height = it->second.height;
    }
    else {
        // Return default size while loading
        sz.width = 100;
        sz.height = 100;
    }
}

void NFX_Container::draw_image(litehtml::uint_ptr hdc, const char* src, const char* baseurl,
    const litehtml::position& pos)
{
    if (!src) return;

    std::string src_str = src;

    std::lock_guard<std::mutex> lock(images_mutex);
    auto it = loaded_images.find(src_str);
    if (it != loaded_images.end() && it->second.loaded && it->second.texture) {
        SDL_Rect dst = { pos.x, pos.y, pos.width, pos.height };
        SDL_RenderCopy(renderer, it->second.texture, nullptr, &dst);
    }
    else {
        // Draw placeholder rectangle
        SDL_SetRenderDrawColor(renderer, 000, 100, 200, 255);
        SDL_Rect placeholder = { pos.x, pos.y, pos.width, pos.height };
        SDL_RenderFillRect(renderer, &placeholder);

        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer, &placeholder);

        // Draw "IMG" text in the center if there's space
        if (pos.width > 30 && pos.height > 15) {
            // You could draw "IMG" text here if you want
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
    if (base_url) {
        current_base_url = base_url;
    }
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