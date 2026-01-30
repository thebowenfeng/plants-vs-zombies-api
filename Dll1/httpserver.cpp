#include "pch.h"
#include "httpserver.h"

httplib::Server server;
void startHTTPServer() {
    server.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
        std::cout << "hi endpoint" << std::endl;
        res.set_content("hello", "text/plain");
        });

    server.listen("0.0.0.0", 8080);
}