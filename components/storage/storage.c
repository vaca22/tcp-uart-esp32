#include "storage.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
 
static char g_ssid[32] ="testssid";
static char g_password[65]="1234567890";
 
void GetWifiParam(void)
{  
    esp_err_t err;
    nvs_handle_t wificonfig_handle;   
    char wifi_ssid[33] = { 0 };
    char wifi_passwd[65] = { 0 };
    size_t len;
 
    err = nvs_open("wificonfig", NVS_READWRITE, &wificonfig_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else 
    {
        printf("opening NVS handle Done\n");
        // Read
        printf("Get ssid and key from NVS ... \n");
        //get ssid
        len = sizeof(wifi_ssid);
        err = nvs_get_str(wificonfig_handle, "ssid", wifi_ssid, &len);
        if(err==ESP_OK)
        {
            printf("Get ssid success!\n");
            printf("ssid=%s\n",wifi_ssid);
            printf("ssid_len=%d\n",len);
        }
        else
        {
           printf("get err =0x%x\n",err);
           printf("Get ssid fail!\n");
        }
        //get password
        len = sizeof(wifi_passwd);
        err = nvs_get_str(wificonfig_handle, "key", wifi_passwd, &len);     
        if(err==ESP_OK)
        {
            printf("Get key success!\n");
            printf("password=%s\n",wifi_passwd);
            printf("password_len=%d\n",len);
        }
        else
        {
           printf("get err =0x%x\n",err);
           printf("Get ssid fail!\n");
        }
        err = nvs_commit(wificonfig_handle);
        printf((err != ESP_OK) ? "nvs_commit Failed!\n" : " nvs_commit Done\n");
    }
    
    // Close
    nvs_close(wificonfig_handle);
}
 
void SetWifiParam(void)
{
    esp_err_t err;
    nvs_handle_t my_handle;  
    
    err = nvs_open("wificonfig", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else 
    {
        printf("opening NVS handle Done\n");
        printf("Set ssid and key from NVS ... \n");
        //write ssid
        err=nvs_set_str(my_handle,"ssid",g_ssid);
        if(err==ESP_OK)
            printf("set ssid success!\n");
        else
        {
            printf("set ssid fail!\n");
        }  
        //write key
        err=nvs_set_str(my_handle,"key",g_password);
        if(err==ESP_OK)
            printf("set password success!\n");
        else
        {
            printf("set password fail!\n");
        }   
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "nvs_commit Failed!\n" : "nvs_commit Done\n");
    }
     // Close
    nvs_close(my_handle);
    printf("--------------------------------------\n");
}


void initNvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
}