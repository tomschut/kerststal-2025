#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP
#include "../scenes/scene_handler.hpp"
#include <esp_http_server.h>
#include <esp_log.h>
#include <sstream>
#include <string>

class WebServer {
public:
    explicit WebServer(SceneHandler* handler)
        : handler_(handler)
        , server_(nullptr)
    {
    }

    void start()
    {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        if (httpd_start(&server_, &config) == ESP_OK) {
            register_uri("/", HTTP_GET, &WebServer::index_handler);
            register_uri("/play", HTTP_POST, &WebServer::play_handler);
            register_uri("/stop", HTTP_POST, &WebServer::stop_handler);
            register_uri("/playcounts", HTTP_GET, &WebServer::playcounts_handler);
            register_uri("/status", HTTP_GET, &WebServer::status_handler);
            ESP_LOGI("webserver", "Webserver started");
        }
    }

private:
    SceneHandler* handler_;
    httpd_handle_t server_;

    static esp_err_t play_handler(httpd_req_t* req)
    {
        auto* self = static_cast<WebServer*>(req->user_ctx);
        char query[32];
        int scene = -1;
        if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
            char param[8];
            if (httpd_query_key_value(query, "scene", param, sizeof(param)) == ESP_OK) {
                scene = atoi(param);
            }
        }
        if (scene >= 0 && scene < self->handler_->nScenes()) {
            self->handler_->playScene(scene);
            httpd_resp_sendstr(req, "OK");
        } else {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid scene");
        }
        return ESP_OK;
    }

    static esp_err_t stop_handler(httpd_req_t* req)
    {
        auto* self = static_cast<WebServer*>(req->user_ctx);
        self->handler_->stopScene();
        httpd_resp_sendstr(req, "Stopped");
        return ESP_OK;
    }

    static esp_err_t playcounts_handler(httpd_req_t* req)
    {
        auto* self = static_cast<WebServer*>(req->user_ctx);
        std::stringstream ss;
        ss << "[";
        for (int i = 0; i < self->handler_->nScenes(); ++i) {
            if (i > 0)
                ss << ",";
            ss << self->handler_->getPlayCount(i);
        }
        ss << "]";
        std::string json = ss.str();
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json.c_str(), json.length());
        return ESP_OK;
    }

    static esp_err_t status_handler(httpd_req_t* req)
    {
        auto* self = static_cast<WebServer*>(req->user_ctx);
        std::stringstream ss;
        ss << "{";
        ss << "\"playing\":" << (self->handler_->isScenePlaying() ? "true" : "false") << ",";
        ss << "\"currentScene\":"
           << (self->handler_->isScenePlaying() ? std::to_string(self->handler_->getCurrentScene()) : "-1");
        ss << "}";
        std::string json = ss.str();
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json.c_str(), json.length());
        return ESP_OK;
    }

    static esp_err_t index_handler(httpd_req_t* req)
    {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    void register_uri(const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*))
    {
        httpd_uri_t config = { .uri = uri, .method = method, .handler = handler, .user_ctx = this };
        httpd_register_uri_handler(server_, &config);
    }

    static constexpr const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Scene Controller</title>
  <meta charset="utf-8">
</head>
<body>
  <h1>ESP32 Scene Controller</h1>
  <button onclick="fetch('/play?scene=0', {method:'POST'})">Play Scene Zakske</button>
  <button onclick="fetch('/play?scene=1', {method:'POST'})">Play Scene Beuk</button>
  <button onclick="fetch('/play?scene=2', {method:'POST'})">Play Scene Herdertjes</button>
  <button onclick="fetch('/stop', {method:'POST'})">Stop Scene</button>
  <pre id="status"></pre>
  <script>
    async function updateStatus() {
      let res = await fetch('/status');
      let json = await res.json();
      document.getElementById('status').textContent = JSON.stringify(json, null, 2);
    }
    setInterval(updateStatus, 1000);
    updateStatus();
  </script>
  <h2>Play Counts</h2>
  <pre id="playcounts"></pre>
  <script>
    async function updatePlayCounts() {
      let res = await fetch('/playcounts');
      let json = await res.json();
      document.getElementById('playcounts').textContent = JSON.stringify(json, null, 2);
    }
    setInterval(updatePlayCounts, 5000);
    updatePlayCounts();
  </script>
</body>
</html>
)rawliteral";
};

#endif // WEB_SERVER_HPP
