#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include <esp_http_server.h>

#include "http_authentication.h"

static const char *TAG = "authenticat example";

static esp_err_t process_get_handler(httpd_req_t *req)
{
#if !CONFIG_HTTP_AUTH_NONE
  esp_err_t result = check_authorisation(req, "joe", "Password1");

  if (result != ESP_OK)
  {
    return (result == ESP_ERR_NOT_FOUND) ? ESP_OK : ESP_FAIL;
  }
#endif

  const char resp_str[] = "Authenticated!";
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t start_web_server(void)
{

  httpd_handle_t server = NULL;

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  /* Use the URI wildcard matching function in order to
   * allow the same handler to respond to multiple different
   * target URIs which match the wildcard scheme */
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting HTTP Server");
  ESP_ERROR_CHECK(httpd_start(&server, &config));

  /* URI handler for to process a GET request */
  httpd_uri_t process_get = {
      .uri = "/*", // Match all URIs of type /path/to/file
      .method = HTTP_GET,
      .handler = process_get_handler,
      .user_ctx = NULL // server_data    // Pass server data as context
  };

  httpd_register_uri_handler(server, &process_get);

  return ESP_OK;
}

static void sta_connected_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data)
{
  ESP_LOGI(TAG, "Station Connected");
  esp_wifi_connect();
}

static void sta_got_ip_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));

  start_web_server();
}

void app_main(void)
{
  // Flash storage initialization
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
  {
    // Error recovery attempt
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // tcpip
  ESP_ERROR_CHECK(esp_netif_init());

  // initialise system event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  /*  wifi stack initialization */
  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t wifi_config = {
      .sta = {
          .ssid = CONFIG_WIFI_SSID,
          .password = CONFIG_WIFI_PASSWORD,
      },
  };

  esp_netif_create_default_wifi_sta();
  ESP_ERROR_CHECK(esp_wifi_init(&config));

  esp_wifi_set_default_wifi_sta_handlers();

  // Register event handlers to handle, sta connected, got ip, and disconnected events
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &sta_connected_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_got_ip_handler, NULL));

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}
