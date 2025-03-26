/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "esp_http_server.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "../mdns/include/mdns.h"

#include "soft-ap.h"
#include "http-server.h"

#include "../mdns/include/mdns.h"

#define CONFIG_AP_SSID            "ESP32-Leonte"
#define CONFIG_AP_PASSWORD        "12345678"

#define CONFIG_ESP_WIFI_SSID      "lab-iot"
#define CONFIG_ESP_WIFI_PASS      "IoT-IoT-IoT"
#define CONFIG_ESP_MAXIMUM_RETRY  5
#define CONFIG_LOCAL_PORT         10001

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

bool wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    // Scan for SSIDs
    ESP_LOGI(TAG, "Scanning for available Wi-Fi networks...");
    wifi_ap_record_t ap_info[20]; // Array to store scan results (max 20 APs)

    uint16_t number = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true)); // true for blocking scan
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info)); // Retrieve scan results
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&number)); // Get number of available APs

    ESP_LOGI(TAG, "Number of networks found: %d", number);

    // Print the SSID and RSSI of each found network
    for (int i = 0; i < number; i++) {
        ESP_LOGI(TAG, "SSID: %s, RSSI: %d", ap_info[i].ssid, ap_info[i].rssi);
    }

    // Optionally, you can connect to one of the found SSIDs based on the scan results
    // For now, let's connect to the one with the strongest signal (max RSSI).
    int best_rssi = -100;
    int best_ap_index = -1;

    for (int i = 0; i < number; i++) {
        if (ap_info[i].rssi > best_rssi) {
            best_rssi = ap_info[i].rssi;
            best_ap_index = i;
        }
    }

    if (best_ap_index >= 0) {
        ESP_LOGI(TAG, "Connecting to SSID: %s with strongest signal: %d dBm", ap_info[best_ap_index].ssid, best_rssi);

        // Set Wi-Fi config for the best network
        strncpy((char*)wifi_config.sta.ssid, (char*)ap_info[best_ap_index].ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char*)wifi_config.sta.password, CONFIG_ESP_WIFI_PASS, sizeof(wifi_config.sta.password) - 1);

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_connect());
    }

    // Waiting for the connection result
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to the best AP SSID:%s password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASS);
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to any AP");
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    return false;
}

static void udp_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = 0;
    int ip_protocol = 0;

    /*
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(CONFIG_PEER_IP_ADDR); // unde #define CONFIG_PEER_IP_ADDR "192.168.89.abc"
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(CONFIG_PEER_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    */

    struct sockaddr_in local_addr;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(CONFIG_LOCAL_PORT);
    ip_protocol = IPPROTO_IP;
    addr_family = AF_INET;

    while(1)
    {
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", CONFIG_LOCAL_PORT);

        while (1) {

            struct sockaddr source_addr;
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, &source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                 inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }

            vTaskDelay(200 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void start_mdns_service()
{
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGI(TAG, "MDNS Init failed: %d\n", err);
        return;
    }
    ESP_LOGI(TAG, "MDNS Init succesfull");
    //set hostname
    mdns_hostname_set("ESP32-Leonte");
    //set default instance
    mdns_instance_name_set("Jhon's ESP32 Thing");
}

void resolve_mdns_host(const char * host_name)
{
    ESP_LOGI(TAG,"Query A: %s.local", host_name);

    esp_ip4_addr_t addr;
    addr.addr = 0;

    esp_err_t err = mdns_query_a(host_name, 2000,  &addr);
    if(err){
        if(err == ESP_ERR_NOT_FOUND){
            ESP_LOGI(TAG,"Host was not found!");
            return;
        }
        ESP_LOGI(TAG,"Query Failed");
        return;
    }

    printf(IPSTR, IP2STR(&addr));
}


void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    // TODO: 3. SSID scanning in STA mode 
    // TODO: 2. Start the web server 
    if (wifi_init_sta()) {
      start_webserver();
     }
    // TODO: 1. Start the softAP mode
    wifi_init_softap();
     // TODO: 4. mDNS init (if there is time left)   
    start_mdns_service();
    resolve_mdns_host("ESP32-Leonte");
}