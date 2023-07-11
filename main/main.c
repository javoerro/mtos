#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp32/rom/crc.h"
#include "mtos.h"

extern const char img_b64[] asm("_binary_img_b64_start");

void mtos_cb(mtos_event_id_t event_id, void* event_data)
{
    char *TAG = "mtos_cb";
    switch(event_id) {
        case MTOS_EVENT_MASTER_CALL: {
            ESP_LOGW(TAG,"MTOS_EVENT_MASTER_CALL");
            break;
        }
        case MTOS_EVENT_MASTER_ANSWERED: {
            ESP_LOGW(TAG,"MTOS_EVENT_MASTER_ANSWERED");
            break;
        }
        case MTOS_EVENT_MASTER_CHUNK_RX: {
            ESP_LOGW(TAG,"MTOS_EVENT_MASTER_CHUNK_RX");
            mtos_event_chunk_t* evt = (mtos_event_chunk_t*)event_data;
            ESP_LOGW(TAG,"chunk_rx_evt_data={\n    .chunk=%.*s...\n,    .size=%u\n,    .count=%u\n,    .name%s\n}",
            16,evt->chunk_rx.chunk,evt->chunk_rx.size,evt->chunk_rx.count,evt->chunk_rx.name);
            break;
        }
        case MTOS_EVENT_MASTER_UPDATED: {
            ESP_LOGW(TAG,"MTOS_EVENT_MASTER_UPDATED");
            char* ptr = NULL; size_t length;
            mtos_grab_mb((char*)event_data,portMAX_DELAY,&ptr,&length);
            ESP_LOGW(TAG,"%s (%u bytes)",ptr,length);
            mtos_return_mb((char*)event_data);
            break;
        }
        case MTOS_EVENT_MASTER_IDLE: {
            ESP_LOGW(TAG,"MTOS_EVENT_MASTER_IDLE");
            break;
        }
        case MTOS_EVENT_MASTER_TIMEOUT: {
            ESP_LOGW(TAG,"MTOS_EVENT_MASTER_TIMEOUT");
            break;
        }
        case MTOS_EVENT_MASTER_ALLOC_ERROR: {
            ESP_LOGW(TAG,"MTOS_EVENT_MASTER_ALLOC_ERROR");
            break;
        }
        case MTOS_EVENT_SLAVE_INITED: {
            ESP_LOGW(TAG,"MTOS_EVENT_SLAVE_INITED");
            break;
        }
        case MTOS_EVENT_SLAVE_DEMANDED: {
            ESP_LOGW(TAG,"MTOS_EVENT_SLAVE_DEMANDED");
            break;
        }
        case MTOS_EVENT_SLAVE_CHUNK_RQ: {
            ESP_LOGW(TAG,"MTOS_EVENT_SLAVE_CHUNK_RQ");
            mtos_event_chunk_t* evt = (mtos_event_chunk_t*)event_data;
            ESP_LOGW(TAG,"chunk_rx_evt_data={\n    .max_size=%u\n,    .tx_size=%u\n,    .resend=%u\n,    .name%s\n}",
            evt->chunk_rq.max_size,evt->chunk_rq.tx_size,evt->chunk_rq.resend,evt->chunk_rq.name);
            break;
        }
        case MTOS_EVENT_SLAVE_RELEASED: {
            ESP_LOGW(TAG,"MTOS_EVENT_SLAVE_RELEASED");
            break;
        }
        case MTOS_EVENT_SLAVE_FINISHED: {
            ESP_LOGW(TAG,"MTOS_EVENT_SLAVE_FINISHED");
            break;
        }
        case MTOS_EVENT_SLAVE_TIMEOUT: {
            ESP_LOGW(TAG,"MTOS_EVENT_SLAVE_TIMEOUT");
            break;
        }
        case MTOS_EVENT_SLAVE_ALLOC_ERROR: {
            ESP_LOGW(TAG,"MTOS_EVENT_SLAVE_ALLOC_ERROR");
            break;
        }
        default: {
            ESP_LOGW(TAG,"MTOS_UNKNOWN_EVENT");
        }
    }
}

#define ROLE_MASTER

#define MILLIS(ini) (((uint32_t)(portTICK_PERIOD_MS*xTaskGetTickCount()))-ini)

void app_main(void)
{
#ifdef ROLE_MASTER
    printf("mode MASTER\n");
    uint8_t slave_idx = 0;
#else
    printf("mode SLAVE\n");
    bool slave_idx = 100;
#endif
    esp_log_level_set("*",ESP_LOG_NONE);
    esp_log_level_set("mtos",ESP_LOG_INFO);
    esp_log_level_set("mtos_cb",ESP_LOG_INFO);
    // esp_log_level_set("mtos_slave",ESP_LOG_INFO);
    // esp_log_level_set("mtos_master",ESP_LOG_INFO);
    // esp_log_level_set("mtos_uart",ESP_LOG_INFO);
    // esp_log_level_set("mtos_strlib",ESP_LOG_INFO);

    mtos_init(mtos_cb);

    // trigger and pattern are the keys that enable connecting the correct data blocks, and they must be pre-shared.
    mtos_new_blob("demo_img",strlen(img_b64)+1,slave_idx,"imgt","imgp");
#ifdef ROLE_MASTER
    vTaskDelay(5000/portTICK_PERIOD_MS);
    mtos_call("demo_img",30000,4096);
#else
    mtos_strcpy("demo_img",img_b64);
    char* ptr = NULL; size_t length;
    mtos_grab_mb("demo_img",portMAX_DELAY,&ptr,&length);
    printf("\n%s (%u bytes)\n",ptr,length);
    mtos_return_mb("demo_img");
#endif
}