#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

#include "esp_event.h"
#include "freertos/semphr.h"
#include <sys/socket.h>
#include "driver/uart.h"
#include "wifi.h"
#include "storage.h"
#include "driver/gpio.h"
static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_22)
#define RXD_PIN (GPIO_NUM_21)

void init(void) {
    const uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
static const char *TAG = "fuckyou";

static int server_socket = 0;                        // 服务器socket
static struct sockaddr_in server_addr;                // server地址
static struct sockaddr_in client_addr;                // client地址
static unsigned int socklen = sizeof(client_addr);    // 地址长度
static int connect_socket = 0;                        // 连接socket
#define TCP_PORT                8888               // 监听客户端端口
bool g_rxtx_need_restart = false;                    // 异常后，重新连接标记


// 建立tcp server
esp_err_t create_tcp_server(bool isCreatServer) {
    //首次建立server
    if (isCreatServer) {
        ESP_LOGI(TAG, "server socket....,port=%d", TCP_PORT);
        server_socket = socket(AF_INET, SOCK_STREAM, 0);//新建socket
        if (server_socket < 0) {
            close(server_socket);//新建失败后，关闭新建的socket，等待下次新建
            return ESP_FAIL;
        }
        //配置新建server socket参数
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TCP_PORT);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //bind:地址的绑定
        if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            close(server_socket);//bind失败后，关闭新建的socket，等待下次新建
            return ESP_FAIL;
        }
    }
    //listen，下次时，直接监听
    if (listen(server_socket, 5) < 0) {
        close(server_socket);//listen失败后，关闭新建的socket，等待下次新建
        return ESP_FAIL;
    }
    //accept，搜寻全连接队列
    connect_socket = accept(server_socket, (struct sockaddr *) &client_addr, &socklen);
    if (connect_socket < 0) {
        close(server_socket);//accept失败后，关闭新建的socket，等待下次新建
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "tcp connection established!");
    return ESP_OK;
}



void close_socket()
{
    close(connect_socket);
    close(server_socket);
}

// 接收数据任务
void recv_data(void *pvParameters)
{
    int len = 0;
    char databuff[1024];
    while (1){
        memset(databuff, 0x00, sizeof(databuff));//清空缓存
        len = recv(connect_socket, databuff, sizeof(databuff), 0);//读取接收数据
        g_rxtx_need_restart = false;
        if (len > 0){
            ESP_LOGI(TAG, "recvData: %s", databuff);//打印接收到的数组
            uart_write_bytes(UART_NUM_1, databuff, len);
//            send(connect_socket, databuff, strlen(databuff), 0);//接收数据回发
            //sendto(connect_socket, databuff , sizeof(databuff), 0, (struct sockaddr *) &remote_addr,sizeof(remote_addr));
        }else{
          //  show_socket_error_reason("recv_data", connect_socket);//打印错误信息
            g_rxtx_need_restart = true;//服务器故障，标记重连
            vTaskDelete(NULL);
        }
    }
    close_socket();
    g_rxtx_need_restart = true;//标记重连
    vTaskDelete(NULL);
}
// 建立TCP连接并从TCP接收数据
static void tcp_connect(void *pvParameters) {
    while (1) {
        g_rxtx_need_restart = false;
        ESP_LOGI(TAG, "start tcp connected");
        TaskHandle_t tx_rx_task = NULL;
        vTaskDelay(3000 / portTICK_RATE_MS);// 延时3S准备建立server
        ESP_LOGI(TAG, "create tcp server");
        int socket_ret = create_tcp_server(true);// 建立server
        if (socket_ret == ESP_FAIL) {// 建立失败
            ESP_LOGI(TAG, "create tcp socket error,stop...");
            continue;
        } else {// 建立成功
            ESP_LOGI(TAG, "create tcp socket succeed...");
            // 建立tcp接收数据任务
            if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task)) {
                ESP_LOGI(TAG, "Recv task create fail!");
            } else {
                ESP_LOGI(TAG, "Recv task create succeed!");
            }
        }
        while (1) {
            vTaskDelay(3000 / portTICK_RATE_MS);
            if (g_rxtx_need_restart) {// 重新建立server，流程和上面一样
                ESP_LOGI(TAG, "tcp server error,some client leave,restart...");
                // 建立server
                if (ESP_FAIL != create_tcp_server(false)) {
                    if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task)) {
                        ESP_LOGE(TAG, "tcp server Recv task create fail!");
                    } else {
                        ESP_LOGI(TAG, "tcp server Recv task create succeed!");
                        g_rxtx_need_restart = false;//重新建立完成，清除标记
                    }
                }
            }
        }
    }
    vTaskDelete(NULL);
}

static void get_ip_yes() {
    ESP_LOGE(TAG, "fuckfuck");
    xTaskCreate(&tcp_connect, "tcp_connect", 4096, NULL, 5, NULL);
    register_got_ip(NULL);
}


//int sendData(const char* logName, const char* data)
//{
//    const int len = strlen(data);
//    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
//    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
//    return txBytes;
//}

//static void tx_task(void *arg)
//{
//    static const char *TX_TASK_TAG = "TX_TASK";
//    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
//    while (1) {
//        sendData(TX_TASK_TAG, "Hello world");
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//    }
//}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            send(connect_socket, data, rxBytes, 0);//接收数据回发
           // ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

void app_main(void) {
    initNvs();

    Wifi_ip_got_callback *wifi_callback;
    wifi_callback = (Wifi_ip_got_callback *) malloc(sizeof(Wifi_ip_got_callback));
    wifi_callback->func_name = get_ip_yes;
    register_got_ip(wifi_callback);
    wifi_init_vaca();


    init();
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    //xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);

}
