#include "browser.h"
#include <httplib.hpp>
#include <iostream>

NFX_Browser::NFX_Browser(SDL_Window* window) : window(window)
{
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!this->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        exit(-1);
    }

    this->container = new NFX_Container(this->renderer);
}

NFX_Browser::~NFX_Browser()
{
    delete this->container;
    SDL_DestroyRenderer(this->renderer);
}

std::string NFX_Browser::get_default_css()
{
    // Basic CSS to make HTML look decent
    return R"(
        body { 
            margin: 10px; 
            font-family: sans-serif; 
            font-size: 16px; 
            line-height: 1.4;
            color: #000;
            background: #fff;
        }
        h1 { 
            font-size: 32px; 
            font-weight: bold; 
            margin: 20px 0 15px 0;
            display: block;
        }
        h2 { 
            font-size: 24px; 
            font-weight: bold; 
            margin: 18px 0 12px 0;
            display: block;
        }
        h3 { 
            font-size: 20px; 
            font-weight: bold; 
            margin: 16px 0 10px 0;
            display: block;
        }
        p { 
            margin: 10px 0; 
            display: block;
        }
        a { 
            color: #0066cc; 
            text-decoration: underline;
        }
        a:hover { 
            color: #0052a3; 
        }
        div { 
            display: block; 
        }
        br { 
            display: block; 
            margin: 5px 0;
        }
        ul, ol { 
            margin: 10px 0; 
            padding-left: 30px;
            display: block;
        }
        li { 
            display: list-item; 
            margin: 5px 0;
        }
    )";
}

void NFX_Browser::load(NFX_Url& url)
{
    this->base_url = url.to_str();

    httplib::SSLClient cli(url.hostname);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(10, 0);
    cli.enable_server_certificate_verification(false);
    cli.set_follow_location(true);
    cli.set_ca_cert_path("");

    auto res = cli.Get(url.path);
    if (res) {
        this->current_html = res->body;

        // Create litehtml document with default CSS
        std::string css = get_default_css();
        //litehtml::context ctx;

        this->document = litehtml::document::createFromString(
            this->current_html.c_str(),
            this->container,
            litehtml::master_css, // &ctx,
            css.c_str()
        );

        if (this->document) {
            // Get window size for rendering
            int window_width = 800; // Default width
            int window_height = 600; // Default height
            SDL_GetWindowSize(this->window, &window_width, &window_height);

            // Render the document to calculate layout
            this->document->render(window_width);

            std::cout << "LiteHTML document loaded and rendered successfully!" << std::endl;
        }
        else {
            std::cout << "Failed to create LiteHTML document" << std::endl;
        }
    }
    else {
        std::cout << "Error loading page: " << (res ? std::to_string(res->status) : "Connection failed") << std::endl;
    }
}

void NFX_Browser::render()
{
    extern bool searchBarActive;
    extern std::string searchText;

    if (!this->renderer) return;

    // Clear screen with white background
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
    SDL_RenderClear(this->renderer);

    // Render the HTML document if available
    if (this->document) {
        litehtml::position clip(0, 0, 0, 0);
        this->container->get_client_rect(clip);

        // Draw the document
        this->document->draw(reinterpret_cast<litehtml::uint_ptr>(this->renderer), 0, 0, &clip);
    }

    // Render search bar if active (keeping your existing functionality)
    if (searchBarActive) {
        // Simple search bar rendering
        SDL_SetRenderDrawColor(this->renderer, 240, 240, 240, 255);
        SDL_Rect searchBar = { 10, 10, 780, 30 };
        SDL_RenderFillRect(this->renderer, &searchBar);

        SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(this->renderer, &searchBar);

        // Render search text (you might want to create a simple text rendering method)
        // For now, this is just a placeholder
    }

    SDL_RenderPresent(this->renderer);
}

void NFX_Browser::on_anchor_click(const std::string& url)
{
    // Handle link clicks - you can expand this to navigate to new URLs
    std::cout << "Navigating to: " << url << std::endl;

    // For now, just load the new URL
    std::string url_copy = url;
    NFX_Url new_url(url_copy);
    if (new_url.is_valid()) {
        load(new_url);
    }
}