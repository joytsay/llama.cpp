#include "cli-client.h"

#include "base64.hpp"
#include "json.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

static std::string read_file(const std::string & path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("cannot open file: " + path);
    }
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad() || data.empty()) {
        throw std::runtime_error("cannot read file or file is empty: " + path);
    }
    return data;
}

int main(int argc, char ** argv) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << " SERVER_URL [IMAGE] [-p PROMPT | -d JSON_OR_FILE]\n";
        return 1;
    }

    try {
        std::string image;
        std::string prompt = "Describe this image.";
        std::string request;
        bool has_prompt = false;
        bool has_request = false;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-p" || arg == "-d") {
                if (i + 1 == argc) {
                    throw std::runtime_error("missing value for " + arg);
                }
                if (arg == "-p") {
                    prompt = argv[++i];
                    has_prompt = true;
                } else {
                    request = argv[++i];
                    has_request = true;
                }
            } else if (!arg.empty() && arg[0] == '-') {
                throw std::runtime_error("unknown option: " + arg);
            } else if (image.empty()) {
                image = arg;
            } else if (!has_prompt) {
                prompt = arg;
                has_prompt = true;
            } else {
                throw std::runtime_error("unexpected argument: " + arg);
            }
        }
        if (has_prompt && has_request) {
            throw std::runtime_error("use either -p or -d");
        }

        common_json body;
        if (has_request) {
            size_t start = request.find_first_not_of(" \t\r\n");
            body = common_json::parse(start != std::string::npos && request[start] == '{' ? request : read_file(request));
            if (!body.is_object() || !body.contains("messages") || !body["messages"].is_array()) {
                throw std::runtime_error("request must be a JSON object with a messages array");
            }
        } else {
            if (image.empty()) {
                throw std::runtime_error("image path is required without -d");
            }
            std::string data = read_file(image);
            body = {
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
                                {"text", prompt}
                            }
                        })}
                    }
                })}
            };
        }
        body["stream"] = false;

        cli_client client;
        client.server_base = argv[1];
        std::cout << client.post("/v1/chat/completions", body.dump()) << '\n';
    } catch (const std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
