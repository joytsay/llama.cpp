#include "cli-client.h"

#include "base64.hpp"
#include "json.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

int main(int argc, char ** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "usage: " << argv[0] << " SERVER_URL IMAGE [PROMPT]\n";
        return 1;
    }

    try {
        std::ifstream file(argv[2], std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot open image: " + std::string(argv[2]));
        }
        std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (file.bad() || data.empty()) {
            throw std::runtime_error("cannot read image or image is empty: " + std::string(argv[2]));
        }

        common_json body = {
            {"stream", false},
            {"max_tokens", 256},
            {"messages", common_json::array({
                {
                    {"role", "user"},
                    {"content", common_json::array({
                        {
                            {"type", "image_url"},
                            {"image_url", {{"url", "data:image/unknown;base64," + base64::encode(data)}}}
                        },
                        {
                            {"type", "text"},
                            {"text", argc == 4 ? argv[3] : "Describe this image."}
                        }
                    })}
                }
            })}
        };

        cli_client client;
        client.server_base = argv[1];
        std::cout << client.post("/v1/chat/completions", body.dump()) << '\n';
    } catch (const std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
