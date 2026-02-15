#include "pch.h"
#include "httpserver.h"
#include "jsonlib.hpp"
#include "plant.h"
#include "seed.h"
#include "game.h"
#include "webhook.h"
#include <regex>

using json = nlohmann::json;

httplib::Server server;

void registerRoutes() {
    server.Put("/api/plant/add", [](const httplib::Request& req, httplib::Response& res) {
        std::map<std::string, std::string> jsonBody = json::parse(req.body);
        int result = addPlant(std::stoi(jsonBody["row"]), std::stoi(jsonBody["col"]), std::stoi(jsonBody["index"]));
        switch (result) {
            case 1:
                res.set_content("Not in game", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            case 2:
                res.set_content("Seed (index) does not exist", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            default:
                break;
        }
        res.status = httplib::StatusCode::OK_200;
    });

    server.Get("/api/seed/bank/size", [](const httplib::Request& req, httplib::Response& res) {
        int result = getSeedBankSize();
        switch (result) {
            case -1:
                res.set_content("Not in game", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            default:
                res.set_content(std::to_string(result), "text/plain");
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Get("/api/seed/bank/selection/type", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("index")) {
            res.set_content("Index not provided", "text/plain");
            res.status = httplib::StatusCode::BadRequest_400;
            return;
        }

        std::string index = req.get_param_value("index");
        int result = getSeedPacketType(std::stoi(index));
        switch (result) {
            case -2:
                res.set_content("Not in game", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            default:
                res.set_content(std::to_string(result), "text/plain");
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Get("/api/seed/bank/selection/size", [](const httplib::Request& req, httplib::Response& res) {
        int result = getSeedInBank();
        switch (result) {
            case -1:
                res.set_content("Not in game", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            default:
                res.set_content(std::to_string(result), "text/plain");
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Post("/api/seed/choose_seed/random", [](const httplib::Request& req, httplib::Response& res) {
        int result = chooseRandomSeeds();
        switch (result) {
            case 1:
                res.set_content("Not choosing seeds", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            default:
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Post("/api/seed/choose_seed/pick_seed", [](const httplib::Request& req, httplib::Response& res) {
        std::map<std::string, std::string> jsonBody = json::parse(req.body);
        int result = chooseSeed(std::stoi(jsonBody["type"]));
        switch (result) {
            case 1:
                res.set_content("Not choosing seeds", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            default:
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Post("/api/game/start", [](const httplib::Request& req, httplib::Response& res) {
        int result = startGame();
        switch (result) {
            case 1:
                res.set_content("Not choosing seeds", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            case 2:
                res.set_content("Not enough seeds chosen", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            default:
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Get("/api/game/state", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(std::to_string(getGameUi()), "text/plain");
        res.status = httplib::StatusCode::OK_200;
    });

    server.Get("/api/game/result", [](const httplib::Request& req, httplib::Response& res) {
        int result = getGameResult();
        switch (result) {
            case 0:
            case 1:
            case 2:
                res.set_content(std::to_string(result), "text/plain");
                res.status = httplib::StatusCode::OK_200;
                return;
            default:
                res.set_content("Unable to get in-game result", "text/plain");
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Post("/api/game/restart", [](const httplib::Request& req, httplib::Response& res) {
        int result = restartSurvivalLevel();
        switch (result) {
            case 1:
                res.set_content("Game not lost", "text/plain");
                res.status = httplib::StatusCode::Conflict_409;
                return;
            default:
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Post("/api/webhook/add", [](const httplib::Request& req, httplib::Response& res) {
        std::map<std::string, std::string> jsonBody = json::parse(req.body);
        std::regex urlRegex("^https?://");
        std::smatch match;

        std::string callbackKey = jsonBody["callbackKey"];
        std::string url = jsonBody["callbackUrl"];
        if (!std::regex_search(url, match, urlRegex)) {
            res.set_content("Invalid callback URL", "text/plain");
            res.status = httplib::StatusCode::BadRequest_400;
            return;
        }

        int result = 0;
        if (callbackKey == "choose_seed") {
            result = insertCallback(CALLBACK_KEY::CHOOSE_SEED, url);
        }
        else if (callbackKey == "game_over") {
            result = insertCallback(CALLBACK_KEY::GAME_OVER, url);
        }
        else {
            res.set_content("Callback key not recognised", "text/plain");
            res.status = httplib::StatusCode::BadRequest_400;
            return;
        }

        switch (result) {
            case 1:
                res.status = httplib::StatusCode::NotModified_304;
                return;
            default:
                res.status = httplib::StatusCode::OK_200;
                return;
        }
    });

    server.Post("/api/webhook/remove", [](const httplib::Request& req, httplib::Response& res) {
        std::map<std::string, std::string> jsonBody = json::parse(req.body);
        std::string callbackKey = jsonBody["callbackKey"];
        std::string url = jsonBody["callbackUrl"];

        int result = 0;
        if (callbackKey == "choose_seed") {
            result = removeCallback(CALLBACK_KEY::CHOOSE_SEED, url);
        }
        else if (callbackKey == "game_over") {
            result = removeCallback(CALLBACK_KEY::GAME_OVER, url);
        }
        else {
            res.set_content("Callback key not recognised", "text/plain");
            res.status = httplib::StatusCode::BadRequest_400;
            return;
        }

        switch (result) {
        case 1:
            res.status = httplib::StatusCode::NotModified_304;
            return;
        default:
            res.status = httplib::StatusCode::OK_200;
            return;
        }
    });
}

void stopServer() {
    server.stop();
}

void startHTTPServer() {
    registerRoutes();
    
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << req.method << " " << req.path << " -> " << res.status << std::endl;
    });
    server.set_error_logger([](const httplib::Error& err, const httplib::Request* req) {
        std::cout << httplib::to_string(err) << " while processing request";
        if (req) {
            std::cout << ", client: " << req->get_header_value("X-Forwarded-For")
                << ", request: '" << req->method << " " << req->path << " " << req->version << "'"
                << ", host: " << req->get_header_value("Host");
        }
        std::cout << std::endl;
    });
    server.set_exception_handler([](const auto& req, auto& res, std::exception_ptr ep) {
        try {
            std::rethrow_exception(ep);
        }
        catch (std::exception& e) {
            res.set_content(e.what(), "text/plain");
        }
        catch (...) { // See the following NOTE
            res.set_content("Unknown Exception", "text/plain");
        }
        res.status = httplib::StatusCode::InternalServerError_500;
    });

    server.listen("0.0.0.0", 8080);
}