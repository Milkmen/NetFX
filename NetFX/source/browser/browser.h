#pragma once

#include <string>
#include <litehtml.h>
#include "renderer/container.h"
#include <SDL.h>

class NFX_Url
{
public:
    std::string schema;
    std::string hostname;
    std::string path;
    int port;

    NFX_Url(std::string& url) {
        parse_url(url);
    }

    std::string to_str() {
        std::string result = schema + "://" + hostname;

        // Add port if it's not the default
        if ((schema == "http" && port != 80) ||
            (schema == "https" && port != 443)) {
            result += ":" + std::to_string(port);
        }

        result += path;
        return result;
    }

    bool is_https() {
        return schema == "https";
    }

    bool is_valid() {
        return !schema.empty() && !hostname.empty();
    }

private:
    void parse_url(const std::string& url) {
        // Default values
        schema = "";
        hostname = "";
        path = "/";
        port = 80;

        if (url.empty()) return;

        std::string remaining = url;

        // Extract schema
        size_t schema_pos = remaining.find("://");
        if (schema_pos != std::string::npos) {
            schema = remaining.substr(0, schema_pos);
            remaining = remaining.substr(schema_pos + 3);

            // Set default port based on schema
            if (schema == "https") {
                port = 443;
            }
            else if (schema == "http") {
                port = 80;
            }
        }
        else {
            // No schema, assume http
            schema = "http";
            port = 80;
        }

        // Extract hostname and optional port
        size_t path_pos = remaining.find('/');
        std::string host_part;

        if (path_pos != std::string::npos) {
            host_part = remaining.substr(0, path_pos);
            path = remaining.substr(path_pos);
        }
        else {
            host_part = remaining;
            path = "/";
        }

        // Check for port in hostname
        size_t port_pos = host_part.find(':');
        if (port_pos != std::string::npos) {
            hostname = host_part.substr(0, port_pos);
            std::string port_str = host_part.substr(port_pos + 1);
            try {
                port = std::stoi(port_str);
            }
            catch (...) {
                // Invalid port, keep default
            }
        }
        else {
            hostname = host_part;
        }

        // Ensure path starts with '/'
        if (!path.empty() && path[0] != '/') {
            path = "/" + path;
        }
    }
};

class NFX_Browser
{
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    NFX_Container* container;
    std::shared_ptr<litehtml::document> document;
    std::string current_html;
    std::string base_url;

    // Default CSS for basic styling
    std::string get_default_css();
    void renderSimpleText(TTF_Font* font, const char* text, int x, int y);
public:
    NFX_Browser(SDL_Window* window);
    ~NFX_Browser();
    void load(NFX_Url& url);
    void render();
    void on_anchor_click(const std::string& url);
    void handle_click(int x, int y);
};