#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

#include "esp_event.h"
#include "freertos/semphr.h"
#include <sys/socket.h>

#include "wifi.h"
#include "storage.h"
static const char *TAG = "fuckyou";
static void good() {
        ESP_LOGE(TAG, "fuckfuck");
}
void app_main(void)
{
    initNvs();
    FUNCSTRUCT *a;
    a= (FUNCSTRUCT*)malloc(sizeof(FUNCSTRUCT));
    a->func_name=good;
    register_got_ip(a);
    wifi_init_vaca();


}
