#include "browser.h"
#include <httplib.hpp>
#include <gumbo.h>

void NFX_Browser::convertGumboToLCRS(LCRS_Tree<NFX_DrawObj_t>& tree, GumboNode* gumboNode, LCRS_Node<NFX_DrawObj_t>* parent) 
{
    if (gumboNode->type != GUMBO_NODE_ELEMENT && gumboNode->type != GUMBO_NODE_TEXT)
        return;

    NFX_DrawObj_t htmlNode;

    if (gumboNode->type == GUMBO_NODE_ELEMENT) {
        const GumboElement& el = gumboNode->v.element;
        htmlNode.tag = gumbo_normalized_tagname(el.tag);

        switch (el.tag)
        {
        case GUMBO_TAG_SCRIPT:
            return;
        case GUMBO_TAG_STYLE:
            return;
        case GUMBO_TAG_TITLE:
        {
            GumboVector* children = &gumboNode->v.element.children;
            std::string title = "WebFX - ";
            title.append(((GumboNode*)children->data[0])->v.text.text);
            SDL_SetWindowTitle(this->renderer->window, title.c_str());
            return;
        }
        default:
            for (unsigned int i = 0; i < el.attributes.length; ++i) {
                GumboAttribute* attr = static_cast<GumboAttribute*>(el.attributes.data[i]);
                htmlNode.attributes[attr->name] = attr->value;
            }
        }
    }
    else if (gumboNode->type == GUMBO_NODE_TEXT) {
        htmlNode.inner = gumboNode->v.text.text;
        htmlNode.tag = "#text"; // special marker for text node
    }

    // Add to tree
    LCRS_Node<NFX_DrawObj_t>* current;
    if (parent == nullptr) {
        current = tree.createRoot(htmlNode); // Root node
    }
    else {
        current = tree.addChild(parent, htmlNode); // Child of previous
    }

    // Recurse over children (if element)
    if (gumboNode->type == GUMBO_NODE_ELEMENT) {
        GumboVector* children = &gumboNode->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            convertGumboToLCRS(tree, static_cast<GumboNode*>(children->data[i]), current);
        }
    }
}

NFX_Browser::NFX_Browser(SDL_Window* window)
{
    this->renderer = new NFX_Renderer(window);
}

NFX_Browser::~NFX_Browser()
{
    if(this->output)
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    delete this->renderer;
}

void NFX_Browser::extract_text(GumboNode* node)
{
    if (node->type == GUMBO_NODE_TEXT) 
    {
        std::string text = node->v.text.text;
        // Skip empty/whitespace-only text
        if (!text.empty() && text.find_first_not_of(" \t\r\n") != std::string::npos) 
        {
            // Trim whitespace
            size_t start = text.find_first_not_of(" \t\r\n");
            size_t end = text.find_last_not_of(" \t\r\n");
            text = text.substr(start, end - start + 1);

            // Check the parent element's tag to determine where this text belongs
            if (node->parent && node->parent->type == GUMBO_NODE_ELEMENT) 
            {
                switch (node->parent->v.element.tag) 
                {
                case GUMBO_TAG_SCRIPT:
                    this->scripts.push_back(node);
                    break;
                case GUMBO_TAG_STYLE:
                    this->stylesheets.push_back(node);
                    break;
                case GUMBO_TAG_TITLE:
                    SDL_SetWindowTitle(this->renderer->window, ("WebFX - " + text).c_str());
                    break;
                default:
                    this->renderer->rendered.push_back(NFX_DrawObj_t{ text });
                }
            }
            else 
            {
                // Text node without element parent - treat as regular content
                this->renderer->rendered.push_back(NFX_DrawObj_t{ text });
            }
        }
    }
    else if (node->type == GUMBO_NODE_ELEMENT) 
    {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) 
        {
            this->extract_text(static_cast<GumboNode*>(children->data[i]));
        }
    }
}

void NFX_Browser::load(NFX_Url& url)
{
    httplib::SSLClient cli(url.hostname);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(10, 0);
    cli.enable_server_certificate_verification(false);
    cli.set_follow_location(true);
    cli.set_ca_cert_path("");

    auto res = cli.Get(url.path);
    if (res) {
        this->output = gumbo_parse(res->body.c_str());

        if (this->output && this->output->root) {
            convertGumboToLCRS(this->renderer->tree, this->output->root);
        }

        std::cout << "Parsed successfully!" << std::endl;
    }
    else {
        std::cout << "Error: " << (res ? std::to_string(res->status) : "Connection failed") << std::endl;
    }
}

void NFX_Browser::render()
{
    this->renderer->render();
}
