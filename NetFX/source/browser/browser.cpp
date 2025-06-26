#include "browser.h"
#include <httplib.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include "../bytesize.h"

NFX_Browser::NFX_Browser(SDL_Window* window) : window(window)
{
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!this->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        exit(-1);
    }

    this->container = new NFX_Container(this->renderer);
    this->container->set_browser(this);
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
    std::cout << "Loading URL: " << url.to_str() << std::endl;

    this->base_url = url.to_str();

    try {
        httplib::Result res;

        // Set user agent to avoid blocking
        httplib::Headers headers = {
            {"User-Agent", "NetFX Browser/1.0"},
            {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
            {"Accept-Language", "en-US,en;q=0.5"},
            {"Accept-Encoding", "identity"},  // Don't accept compressed content for simplicity
            {"Connection", "close"}  // Don't keep connection alive
        };

        std::cout << "Connecting to " << url.hostname << ":" << url.port << std::endl;

        // Handle HTTPS and HTTP separately
        if (url.is_https()) {
            httplib::SSLClient ssl_cli(url.hostname, url.port);

            // Configure SSL client
            ssl_cli.enable_server_certificate_verification(false);
            ssl_cli.set_ca_cert_path("");

            // Set timeouts
            ssl_cli.set_connection_timeout(10, 0);  // 10 seconds
            ssl_cli.set_read_timeout(15, 0);        // 15 seconds
            ssl_cli.set_write_timeout(10, 0);       // 10 seconds
            ssl_cli.set_follow_location(true);

            res = ssl_cli.Get(url.path, headers);
        }
        else {
            httplib::Client http_cli(url.hostname, url.port);

            // Set timeouts
            http_cli.set_connection_timeout(10, 0);  // 10 seconds
            http_cli.set_read_timeout(15, 0);        // 15 seconds
            http_cli.set_write_timeout(10, 0);       // 10 seconds
            http_cli.set_follow_location(true);

            res = http_cli.Get(url.path, headers);
        }

        if (res) {
            std::cout << "HTTP Status: " << res->status << std::endl;

            if (res->status == 200) {
                this->current_html = res->body;
                std::cout << "Downloaded " << this->current_html.length() << " bytes" << std::endl;

                // Limit HTML size to prevent memory issues
                const size_t MAX_HTML_SIZE = 8 * MiB;
                if (this->current_html.length() > MAX_HTML_SIZE) {
                    std::cout << "HTML too large, truncating..." << std::endl;
                    this->current_html = this->current_html.substr(0, MAX_HTML_SIZE);
                }

                // Create litehtml document with default CSS
                std::string css = get_default_css();

                try {
                    this->document = litehtml::document::createFromString(
                        this->current_html.c_str(),
                        this->container,
                        litehtml::master_css,
                        css.c_str()
                    );

                    if (this->document) {
                        // Get window size for rendering
                        int window_width = 800;
                        int window_height = 600;
                        SDL_GetWindowSize(this->window, &window_width, &window_height);

                        // Render the document to calculate layout
                        this->document->render(window_width);

                        std::cout << "LiteHTML document loaded and rendered successfully!" << std::endl;
                    }
                    else {
                        std::cout << "Failed to create LiteHTML document" << std::endl;
                        this->current_html = "<html><body><h1>Error</h1><p>Failed to parse HTML document</p></body></html>";
                    }
                }
                catch (const std::exception& e) {
                    std::cout << "Exception creating LiteHTML document: " << e.what() << std::endl;
                    this->current_html = "<html><body><h1>Parse Error</h1><p>Failed to parse HTML: " + std::string(e.what()) + "</p></body></html>";
                }
            }
            else if (res->status >= 300 && res->status < 400) {
                std::cout << "Redirect status: " << res->status << std::endl;
                // httplib should handle redirects automatically, but if we get here it means redirect failed
                this->current_html = "<html><body><h1>Redirect Error</h1><p>Too many redirects or redirect failed</p></body></html>";
            }
            else {
                std::cout << "HTTP Error: " << res->status << std::endl;
                this->current_html = "<html><body><h1>HTTP Error " + std::to_string(res->status) + "</h1><p>Failed to load page</p></body></html>";
            }
        }
        else {
            auto err = res.error();
            std::cout << "Connection error: " << httplib::to_string(err) << std::endl;

            std::string error_msg;
            switch (err) {
            case httplib::Error::Connection:
                error_msg = "Connection failed - check if the server is reachable";
                break;
            case httplib::Error::BindIPAddress:
                error_msg = "Failed to bind IP address";
                break;
            case httplib::Error::Read:
                error_msg = "Read timeout or connection lost";
                break;
            case httplib::Error::Write:
                error_msg = "Write timeout or connection lost";
                break;
            case httplib::Error::ExceedRedirectCount:
                error_msg = "Too many redirects";
                break;
            case httplib::Error::Canceled:
                error_msg = "Request was canceled";
                break;
            case httplib::Error::SSLConnection:
                error_msg = "SSL connection failed";
                break;
            case httplib::Error::SSLLoadingCerts:
                error_msg = "SSL certificate loading failed";
                break;
            case httplib::Error::SSLServerVerification:
                error_msg = "SSL server verification failed";
                break;
            case httplib::Error::UnsupportedMultipartBoundaryChars:
                error_msg = "Unsupported multipart boundary characters";
                break;
            default:
                error_msg = "Unknown connection error";
                break;
            }

            this->current_html = "<html><body><h1>Connection Error</h1><p>" + error_msg + "</p><p>URL: " + url.to_str() + "</p></body></html>";
        }

    }
    catch (const std::exception& e) {
        std::cout << "Exception during load: " << e.what() << std::endl;
        this->current_html = "<html><body><h1>Exception</h1><p>Error loading page: " + std::string(e.what()) + "</p></body></html>";
    }
    catch (...) {
        std::cout << "Unknown exception during load" << std::endl;
        this->current_html = "<html><body><h1>Unknown Error</h1><p>An unknown error occurred while loading the page</p></body></html>";
    }

    // If we still don't have a document, create a simple error page
    if (!this->document && !this->current_html.empty()) {
        try {
            std::string css = get_default_css();
            this->document = litehtml::document::createFromString(
                this->current_html.c_str(),
                this->container,
                litehtml::master_css,
                css.c_str()
            );

            if (this->document) {
                int window_width = 800;
                int window_height = 600;
                SDL_GetWindowSize(this->window, &window_width, &window_height);
                this->document->render(window_width);
            }
        }
        catch (...) {
            std::cout << "Failed to create error page document" << std::endl;
        }
    }
}

void NFX_Browser::renderSimpleText(TTF_Font* font, const char* text, int x, int y)
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
        try {
            litehtml::position clip(0, 0, 0, 0);
            this->container->get_client_rect(clip);

            // Draw the document
            this->document->draw(reinterpret_cast<litehtml::uint_ptr>(this->renderer), 0, 0, &clip);
        }
        catch (const std::exception& e) {
            std::cout << "Exception during render: " << e.what() << std::endl;
        }
        catch (...) {
            std::cout << "Unknown exception during render" << std::endl;
        }
    }

    // Render search bar if active
    if (searchBarActive) {
        // Simple search bar rendering
        SDL_SetRenderDrawColor(this->renderer, 240, 240, 240, 255);
        SDL_Rect searchBar = { 10, 10, 780, 30 };
        SDL_RenderFillRect(this->renderer, &searchBar);
        SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(this->renderer, &searchBar);
        this->renderSimpleText(this->container->fonts["default"], searchText.c_str(), 14, 14);
    }

    SDL_RenderPresent(this->renderer);
}

void NFX_Browser::on_anchor_click(const std::string& url)
{
    std::cout << "Navigating to: " << url << std::endl;

    try {
        std::string url_copy = url;
        NFX_Url new_url(url_copy);
        if (new_url.is_valid()) {
            load(new_url);
        }
        else {
            std::cout << "Invalid URL: " << url << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cout << "Exception in anchor click: " << e.what() << std::endl;
    }
}

std::shared_ptr<litehtml::element> find_element_at(std::shared_ptr<litehtml::element> root, int x, int y)
{
    if (!root) return nullptr;

    const litehtml::position& pos = root->get_placement();

    // First check children (depth-first), in reverse order for Z-index correctness
    const auto& children = root->children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto hit = find_element_at(*it, x, y);
        if (hit) return hit;
    }

    // Then check self
    if (x >= pos.left() && x <= pos.right() &&
        y >= pos.top() && y <= pos.bottom()) {
        return root;
    }

    return nullptr;
}


void NFX_Browser::handle_click(int x, int y)
{
    if (!this->document) return;

    try {
        litehtml::position::vector redraw_boxes;
        this->document->on_lbutton_down(x, y, 0, 0, redraw_boxes);

        auto root = this->document->root();
        auto clicked_element = find_element_at(root, x, y);

        if (clicked_element) {
            auto current_element = clicked_element;

            while (current_element) {
                std::string tag = current_element->get_tagName();
                if (tag == "a") { // This check always fails even if i click on a <a>
                    const char* href = current_element->get_attr("href");

                    if (href && *href) {
                        std::string url_str = href;

                        if (url_str[0] == '/') {
                            NFX_Url current_url(this->base_url);
                            url_str = current_url.schema + "://" + current_url.hostname;
                            if ((current_url.schema == "http" && current_url.port != 80) ||
                                (current_url.schema == "https" && current_url.port != 443)) {
                                url_str += ":" + std::to_string(current_url.port);
                            }
                            url_str += href;
                        }
                        else if (url_str.find("://") == std::string::npos) {
                            if (!this->base_url.empty() && this->base_url.back() != '/')
                                url_str = this->base_url + "/" + url_str;
                            else
                                url_str = this->base_url + url_str;
                        }

                        std::cout << "Link clicked: " << url_str << std::endl;
                        this->on_anchor_click(url_str);
                        break;
                    }
                }

                current_element = current_element->parent();
            }
        }

        for (const auto& box : redraw_boxes) {
            // Optional: implement partial redraws
        }
    }
    catch (const std::exception& e) {
        std::cout << "Exception in handle_click: " << e.what() << std::endl;
    }
}
