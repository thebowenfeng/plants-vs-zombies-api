#include "pch.h"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "webhook.h"
#include "httplib.h"

std::unordered_map <int, std::unordered_set<std::string>> callbackMap;

int insertCallback(CALLBACK_KEY callbackKey, std::string url) {
    if (callbackMap.count(callbackKey)) {
        if (callbackMap[callbackKey].count(url)) return 1;
        callbackMap[callbackKey].insert(url);
        return 0;
    }
    callbackMap[callbackKey] = std::unordered_set<std::string>();
    callbackMap[callbackKey].insert(url);
    return 0;
}

int removeCallback(CALLBACK_KEY callbackKey, std::string url) {
    if (!callbackMap.count(callbackKey) || !callbackMap[callbackKey].count(url)) return 1;
    callbackMap[callbackKey].erase(url);
    return 0;
}

void setLogger(httplib::Client& cli) {
    cli.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << "Client: " << req.method << " " << req.path
            << " -> " << res.status << " (" << res.body.size() << " bytes, "
            << std::endl;
    });

    cli.set_error_logger([](const httplib::Error& err, const httplib::Request* req) {
        std::cerr << "Client: ";
        if (req) {
            std::cerr << req->method << " " << req->path << " ";
        }
        std::cerr << "failed: " << httplib::to_string(err);

        // Add specific guidance based on error type
        switch (err) {
        case httplib::Error::Connection:
            std::cerr << " (verify server is running and reachable)";
            break;
        case httplib::Error::SSLConnection:
            std::cerr << " (check SSL certificate and TLS configuration)";
            break;
        case httplib::Error::ConnectionTimeout:
            std::cerr << " (increase timeout or check network latency)";
            break;
        case httplib::Error::Read:
            std::cerr << " (server may have closed connection prematurely)";
            break;
        default:
            break;
        }
        std::cerr << std::endl;
    });
}

void extractHostAndPath(const std::string& url, std::string& host, std::string& path) {
    size_t scheme_end = url.find("://");
    std::string protocol = "";
    std::string after_scheme;

    if (scheme_end != std::string::npos) {
        // 1. Extract protocol (e.g., "https")
        protocol = url.substr(0, scheme_end);
        after_scheme = url.substr(scheme_end + 3); // Skip "://"
    }
    else {
        after_scheme = url;
    }

    // 2. Find start of path
    size_t path_start = after_scheme.find_first_of("/");

    if (path_start != std::string::npos) {
        // This captures everything between the "://" and the "/"
        // This naturally includes the port (e.g., "localhost:8080")
        host = after_scheme.substr(0, path_start);
        path = after_scheme.substr(path_start);
    }
    else {
        host = after_scheme;
        path = "/";
    }

    // 3. Attach protocol in front of host
    // The port is already preserved in 'host' from the logic above.
    if (!protocol.empty()) {
        host = protocol + "://" + host;
    }
}

void invokeCallbacks(InvokeCallbackParam* param) {
    if (!callbackMap.count(param->callbackKey)) return;
    try {
        for (const std::string url : callbackMap[param->callbackKey]) {
            std::string host, path;
            extractHostAndPath(url, host, path);
            std::cout << "[Debug] Callback url host: " << host << " path: " << path << std::endl;
            httplib::Client client(host);
            setLogger(client);
            client.Post(path, param->payload, "application/json");
        }
        delete param;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}