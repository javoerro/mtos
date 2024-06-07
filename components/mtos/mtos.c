#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp32/rom/crc.h"
#include "typedefs.h"


typedef enum {
    MTOS_STRCAT,
    MTOS_STRCHR,
    MTOS_STRCMP,
    MTOS_STRCPY,
    MTOS_STRLEN,
    MTOS_STRNCAT,
    MTOS_STRNCMP,
    MTOS_STRNCPY,
    MTOS_STRPBRK,
    MTOS_STRRCHR,
    MTOS_STRSTR,
    MTOS_STRTOK,
    MTOS_MEMSET,
    MTOS_MEMCPY,
    MTOS_MEMMOVE
} mtos_fnc_idx_t;

//maquina de estados de recepcion de datos por uart
typedef enum {
    MTOS_MASTER_ABORT,
    MTOS_MASTER_IDLE,
    MTOS_MASTER_INIT,
    MTOS_MASTER_CHUNK,
    MTOS_MASTER_ENDING
} mtos_master_status_t;

typedef enum {
    MTOS_SLAVE_ABORT,
    MTOS_SLAVE_IDLE,
    MTOS_SLAVE_INIT,
    MTOS_SLAVE_CHUNK,
    MTOS_SLAVE_ENDING
} mtos_slave_status_t;

//union para manejar crc32
typedef union {
    uint32_t value;
    uint8_t raw[4];
} mtos_crc32_t;

// header que antecede una trama de datos enviados por uart
// con sus diferentes usos
typedef union {
    struct __attribute__((packed)) {
        uint16_t max_size;
        uint8_t resend; // 0 solicita proximo chunk, cualquier otro valor solicita la retransmicion del ultimo
        uint8_t crc8;
    } chunk_request;
    struct __attribute__((packed)) {
        uint16_t size;
        uint8_t count;
        uint8_t crc8;
    } chunk_response;
    struct __attribute__((packed)) {
        uint32_t payload_length:24;
        uint32_t crc8:8;
    } trigger_response;
    uint8_t raw[4];
    uint32_t uint32;
} mtos_header_t;

//nodo de una lista enlazada que lleva cuenta de bloques de memoria
//compartidos entre dos equipos, uno local y otro remoto
typedef struct mtos_node {
    void* ptr;
    size_t size;
    size_t length;
    bool blob;
    uint8_t slave;
    mtos_crc32_t crc32;
    char trigger[8];
    char pattern[8];
    char name[16];
    SemaphoreHandle_t smphr;
    struct mtos_node* next;
} mtos_list_t; //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<REALIZAR VERSION I2C CON ASISTENCIA

// receptor de chunk
typedef struct {
    uint8_t* chunk;
    size_t size;
    mtos_crc32_t crc32;
} mtos_chunk_vessel_t;

// receptor de chunk
typedef struct {
    void* buffer;
    size_t* rx_bytes;
    void* header;
    TaskHandle_t th;
} mtos_uart_vessel_t;

static const char *TAG = "mtos";

static mtos_list_t* mtos_list_head = NULL;

static mtos_list_t* mtos_lookup(char name[16])
{
    mtos_list_t* retval = mtos_list_head;
    while (retval != NULL) {
        if (strcmp(retval->name, name) == 0) {
            break;
        }
        retval = retval->next;
    }
    return retval;
}

static void* mtos_strlib_wrap(char name[16], void *src, size_t n, mtos_fnc_idx_t fnc)
{
    char *TAG = "mtos_strlib";
    mtos_list_t* node = mtos_lookup(name);
    if (node != NULL) {
        void* dest = node->ptr;
        void* retval = NULL;
        bool changed = false;
        xSemaphoreTake(node->smphr, portMAX_DELAY);
        ESP_LOGI(TAG,"%s's semaphore taken",node->name);
        switch (fnc) {
            case MTOS_STRCAT:
                ESP_LOGI(TAG,"MTOS_STRCAT");
                retval = strcat((char*)dest, (const char*)src);
                changed = true;
                break;
                // char * strcat ( char * destination, const char * source );
            case MTOS_STRCHR:
                ESP_LOGI(TAG,"MTOS_STRCHR");
                retval = strchr((const char*)dest, *(const char*)src);
                break;
                // const char * strchr ( const char * str, int character );
            case MTOS_STRCMP:
                ESP_LOGI(TAG,"MTOS_STRCMP");
                retval = (void*)strcmp((const char*)dest, (const char*)src);
                break;
                // int strcmp ( const char * str1, const char * str2 );
            case MTOS_STRCPY:
                ESP_LOGI(TAG,"MTOS_STRCPY");
                retval = strcpy((char*)dest, (const char*)src);
                changed = true;
                break;
                // char * strcpy ( char * destination, const char * source );
            case MTOS_STRLEN:
                ESP_LOGI(TAG,"MTOS_STRLEN");
                retval = (void*)strlen((const char*)src);
                break;
                // size_t strlen ( const char * str );
            case MTOS_STRNCAT:
                ESP_LOGI(TAG,"MTOS_STRNCAT");
                retval = strncat((char*)dest, (const char*)src, n);
                changed = true;
                break;
                // char * strncat ( char * destination, const char * source, size_t num );
            case MTOS_STRNCMP:
                ESP_LOGI(TAG,"MTOS_STRNCMP");
                retval = (void*)strncmp((const char*)dest, (const char*)src, n);
                break;
                // int strncmp ( const char * str1, const char * str2, size_t num );
            case MTOS_STRNCPY:
                ESP_LOGI(TAG,"MTOS_STRNCPY");
                retval = strncpy((char*)dest, (const char*)src, n);
                changed = true;
                break;
                // char * strncpy ( char * destination, const char * source, size_t num );
            case MTOS_STRPBRK:
                ESP_LOGI(TAG,"MTOS_STRPBRK");
                retval = strpbrk((const char*)dest, (const char*)src);
                break;
                // const char * strpbrk ( const char * str1, const char * str2 );
            case MTOS_STRRCHR:
                ESP_LOGI(TAG,"MTOS_STRPBRK");
                retval = strrchr((const char*)dest, *(const char*)src);
                break;
                // const char * strrchr ( const char * str, int character );
            case MTOS_STRSTR:
                retval = strstr((const char*)dest, (const char*)src);
                break;
                // const char * strstr ( const char * str1, const char * str2 );
            case MTOS_STRTOK:
                ESP_LOGI(TAG,"MTOS_STRTOK");
                retval = strtok((char*)dest, (const char*)src);
                changed = true;
                break;
                // char * strtok ( char * str, const char * delimiters );
            case MTOS_MEMSET:
                ESP_LOGI(TAG,"MTOS_MEMSET");
                retval = memset(dest, *(int*)src, n);
                changed = true;
                break;
                //void * memset ( void * ptr, int value, size_t num );
            case MTOS_MEMCPY:
                ESP_LOGI(TAG,"MTOS_MEMCPY");
                retval = memcpy(dest, src, n);
                break;
                // void * memcpy ( void * destination, const void * source, size_t num );
            case MTOS_MEMMOVE:
                ESP_LOGI(TAG,"MTOS_MEMMOVE");
                retval = memmove(dest, src, n);
                break;
                //void * memmove ( void * destination, const void * source, size_t num );
        }
        if (changed) {
            node->crc32.value = crc32_be(0,node->ptr,node->length);
        }
        xSemaphoreGive(node->smphr);
        ESP_LOGI(TAG,"%s's semaphore given",node->name);
        return retval;
    }
    return NULL;

}

int mtos_new_blob(char name[16], size_t length, uint8_t slave, char trigger[8], char pattern[8])
{
    //Use for blobs
    ESP_LOGI(TAG,"mtos_new_blob");
    mtos_list_t* current_node = mtos_list_head;
    size_t entries = 0;
    while (current_node != NULL) {
        if (strcmp(current_node->name, name) == 0) {
            ESP_LOGI(TAG,"name already as entry in the list");
            return -3; //return -3 to tell name already exists
        }
        current_node = current_node->next;
        entries++;
    }
    if (entries) {
        ESP_LOGI(TAG,"%u entr%s in the list",entries,(entries == 1 ? "\b\b\b\b\b\bone entry" : "ies"));
    }
    else {
        ESP_LOGI(TAG,"the list is empty");
    }
    mtos_list_t* new_node = (mtos_list_t*)calloc(1,sizeof(mtos_list_t));
    if (new_node) {
        ESP_LOGI(TAG,"node malloc ok");
        new_node->size = 0;
        new_node->length = length;
        new_node->blob = true;
        new_node->slave = slave;
        memcpy(new_node->trigger,trigger,sizeof(((mtos_list_t*)0)->trigger));
        memcpy(new_node->pattern,pattern,sizeof(((mtos_list_t*)0)->pattern));
        memset(new_node->crc32.raw,0,sizeof(((mtos_crc32_t*)0)->raw));
        strcpy(new_node->name,name);
        new_node->ptr = malloc(new_node->length);
        if (new_node->ptr) {
            ESP_LOGI(TAG,"mb malloc ok");
            new_node->smphr = xSemaphoreCreateMutex();
            new_node->next = NULL;

            new_node->crc32.value = crc32_be(0,new_node->ptr,new_node->length);
            
            entries = 0;
            if (mtos_list_head) {
                current_node = mtos_list_head;
                while (current_node->next != NULL) {
                    current_node = current_node->next;
                    entries++;
                }
                ESP_LOGI(TAG,"node placed in list at #%u",entries);
                current_node->next = new_node;
            }
            else {
                ESP_LOGI(TAG,"list initied");
                mtos_list_head = new_node;
            }
            ESP_LOGI(TAG,"node: {\n"
                "   .ptr: *(%p) = %.*s...,\n"
                "   .size: %u,\n"
                "   .length: %u,\n"
                "   .blob: %s,\n"
                "   .slave: %02X,\n"
                "   .crc32: %08X,\n"
                "   .trigger: %s,\n"
                "   .pattern: %s,\n"
                "   .name: %s,\n"
                "}\n",new_node->ptr,5,(char*)new_node->ptr,
                new_node->size,new_node->length,
                new_node->blob ? "true" : "false",
                new_node->slave,
                new_node->crc32.value,new_node->trigger,
                new_node->pattern,new_node->name
            );
        }
        else {
            ESP_LOGI(TAG,"mb alloc error");
            free(new_node);
            return -2;
        }
    }
    else {
        ESP_LOGI(TAG,"node alloc error");
        return -1;
    }
    return 0;
}

int mtos_new_array(char name[16], size_t n, size_t size, uint8_t slave, char trigger[8], char pattern[8])
{
    //Use for arrays
    ESP_LOGI(TAG,"mtos_new_array");
    mtos_list_t* current_node = mtos_list_head;
    size_t entries = 0;
    while (current_node != NULL) {
        if (strcmp(current_node->name, name) == 0) {
            ESP_LOGI(TAG,"name already as entry in the list");
            return -3; //return -3 to indicate name already exists
        }
        current_node = current_node->next;
        entries++;
    }
    if (entries) {
        ESP_LOGI(TAG,"%u entr%s in the list",entries,(entries == 1 ? "\b\b\b\b\b\bone entry" : "ies"));
    }
    else {
        ESP_LOGI(TAG,"the list is empty");
    }
    mtos_list_t* new_node = (mtos_list_t*)calloc(1,sizeof(mtos_list_t));
    if (new_node) {
        ESP_LOGI(TAG,"node malloc ok");
        ESP_LOGI(TAG,"node: %p",new_node);
        new_node->size = size;
        new_node->length = n*size;
        new_node->blob = false;
        new_node->slave = slave;
        memcpy(new_node->trigger,trigger,sizeof(((mtos_list_t*)0)->trigger));
        memcpy(new_node->pattern,pattern,sizeof(((mtos_list_t*)0)->pattern));
        memset(new_node->crc32.raw,0,sizeof(((mtos_crc32_t*)0)->raw));
        strcpy(new_node->name,name);
        new_node->ptr = calloc(n,size);
        if (new_node->ptr) {
            ESP_LOGI(TAG,"array calloc ok");
            new_node->smphr = xSemaphoreCreateMutex();
            new_node->next = NULL;

            new_node->crc32.value = crc32_be(0,new_node->ptr,new_node->length);
            
            entries = 0;
            if (mtos_list_head) {
                current_node = mtos_list_head;
                while (current_node->next != NULL) {
                    current_node = current_node->next;
                }
                ESP_LOGI(TAG,"node placed in list at #%u",entries);
                current_node->next = new_node;
            }
            else {
                ESP_LOGI(TAG,"list initied");
                mtos_list_head = new_node;
            }
            ESP_LOGI(TAG,"node: {\n"
                "   .ptr: *(%p) = %.*s...,\n"
                "   .size: %u,\n"
                "   .length: %u,\n"
                "   .blob: %s,\n"
                "   .slave: %02X,\n"
                "   .crc32: %08X,\n"
                "   .trigger: %s,\n"
                "   .pattern: %s,\n"
                "   .name: %s,\n"
                "}\n",new_node->ptr,5,(char*)new_node->ptr,
                new_node->size,new_node->length,
                new_node->blob ? "true" : "false",
                new_node->slave,
                new_node->crc32.value,new_node->trigger,
                new_node->pattern,new_node->name
            );
        }
        else {
            ESP_LOGI(TAG,"array alloc error");
            free(new_node);
            return -2;
        }
    }
    else {
        ESP_LOGI(TAG,"node alloc error");
        return -1;
    }
    return 0;
}

int mtos_grab_mb(char name[16], TickType_t ticks, void** ptr, size_t* length)
{
    mtos_list_t* node = mtos_lookup(name);
    if (node != NULL) {
        if (xSemaphoreTake(node->smphr, ticks) == pdTRUE) {
                *ptr = node->ptr;
                *length = node->length;
                return 0;
        }
        else {
            return -2;
        }

    }
    else {
        return -1;
    }
}

int mtos_return_mb(char name[16])
{
    mtos_list_t* node = mtos_lookup(name);
    if (node != NULL) {
        node->crc32.value = crc32_be(0,node->ptr,node->length);
        if (xSemaphoreGive(node->smphr) == pdTRUE) {
            return 0;
        }
        else {
            return -2;
        }
    }
    else {
        return -1;
    }
}

int mtos_resize(char name[16], size_t n)
{
    mtos_list_t* node = mtos_lookup(name);
    int retval = -2;
    if (node != NULL) {
        retval = -1;
        xSemaphoreTake(node->smphr, portMAX_DELAY);
        void* new_ptr = realloc(node->ptr,n);
        if (new_ptr) {
            retval = 0;
            node->ptr = new_ptr;
            node->length = n;
            node->crc32.value = crc32_be(0,node->ptr,node->length);
        }
        xSemaphoreGive(node->smphr);
    }
    return retval;
}

char* mtos_strcat(char name[16], const char* src)
{
    // char * strcat ( char * destination, const char * source );
    return (char*)mtos_strlib_wrap(name, (void*)src, 0, MTOS_STRCAT);
}

const char* mtos_strchr(char name[16], char const chr)
{
    // const char * strchr ( const char * str, int character );
    return (const char*)mtos_strlib_wrap(name, (void*)(&chr), 0, MTOS_STRCHR);
}

int mtos_strcmp(char name[16], const char* src)
{
    // int strcmp ( const char * str1, const char * str2 );
    return (int)mtos_strlib_wrap(name, (void*)src, 0, MTOS_STRCMP);
}

char* mtos_strcpy(char name[16], const char* src)
{
    // char * strcpy ( char * destination, const char * source );
    return (char*)mtos_strlib_wrap(name, (void*)src, 0, MTOS_STRCPY);
}

size_t mtos_strlen(char name[16])
{
    // size_t strlen ( const char * str );
    return (size_t)mtos_strlib_wrap(name, NULL, 0, MTOS_STRLEN);
}

char* mtos_strncat(char name[16], const char* src, size_t n)
{
    // char * strncat ( char * destination, const char * source, size_t num );
    return (char*)mtos_strlib_wrap(name, (void*)src, n, MTOS_STRNCAT);
}

int mtos_strncmp(char name[16], const char* src, size_t n)
{
    // int strncmp ( const char * str1, const char * str2, size_t num );
    return (int)mtos_strlib_wrap(name, (void*)src, n, MTOS_STRNCMP);
}

char* mtos_strncpy(char name[16], const char* src, size_t n)
{
    // char * strncpy ( char * destination, const char * source, size_t num );
    return (char*)mtos_strlib_wrap(name, (void*)src, n, MTOS_STRNCPY);
}

const char * mtos_strpbrk(char name[16], const char* src)
{
    // const char * strpbrk ( const char * str1, const char * str2 );
    return (const char *)mtos_strlib_wrap(name, (void*)src, 0, MTOS_STRPBRK);
}

const char * mtos_strrchr(char name[16], char const chr)
{
    // const char * strrchr ( const char * str, int character );
    return (const char *)mtos_strlib_wrap(name, (void*)(&chr), 0,MTOS_STRRCHR);
}

const char * mtos_strstr(char name[16], const char* src)
{
    // const char * strstr ( const char * str1, const char * str2 );
    return (const char * )mtos_strlib_wrap(name, (void*)src, 0,MTOS_STRSTR);
}

char * mtos_strtok(char name[16], const char* src)
{
    // char * strtok ( char * str, const char * delimiters );
    return (char *)mtos_strlib_wrap(name, (void*)src, 0, MTOS_STRTOK);
}

void* mtos_memset(char name[16], char const chr, size_t n)
{
    //void * memset ( void * ptr, int value, size_t num );
    return (void*)mtos_strlib_wrap(name, (void*)(&chr), n, MTOS_MEMSET);
}

void* mtos_memcpy(char name[16], const void* src, size_t n)
{
    // void * memcpy ( void * destination, const void * source, size_t num );
    return (void*)mtos_strlib_wrap(name, (void*)src, n, MTOS_MEMCPY);
}

void* mtos_memmove(char name[16], const void* src, size_t n)
{
    //void * memmove ( void * destination, const void * source, size_t num );
    return (void*)mtos_strlib_wrap(name, (void*)src, n, MTOS_MEMMOVE);
}

int mtos_get_length(char name[16])
{
    mtos_list_t* node = mtos_lookup(name);
    if (node != NULL) {
        return node->length;
    }
    return -1;
}

int mtos_borrow_element(char name[16], void* element, size_t index)
{
    mtos_list_t* node = mtos_lookup(name);
    if (node != NULL) {
        if(!node->blob) {
            size_t raw_idx = index*node->size;
            if (raw_idx < node->length) {
                xSemaphoreTake(node->smphr, portMAX_DELAY);
                memcpy(element,node->ptr+raw_idx,node->size);
                xSemaphoreGive(node->smphr);
                return 0;
            }
            else {
                return -3;
            }
        }
        else {
            return -2;
        }
    }
    else {
        return -1;
    }
}

int mtos_return_element(char name[16], void* element, size_t index)
{
    mtos_list_t* node = mtos_lookup(name);
    if (node != NULL) {
        if(!node->blob) {
            size_t raw_idx = index*node->size;
            if (raw_idx < node->length) {
                xSemaphoreTake(node->smphr, portMAX_DELAY);
                memcpy(node->ptr+raw_idx,element,node->size);
                xSemaphoreGive(node->smphr);
                return 0;
            }
            else {
                return -3;
            }
        }
        else {
            return -2;
        }
    }
    else {
        return -1;
    }
}

#define MILLIS(ini) (((uint32_t)(portTICK_PERIOD_MS*xTaskGetTickCount()))-ini)
#define MTOS_PORT CONFIG_MTOS_UART_PORT
#define MTOS_BUFFER_EFFECTIVE (((CONFIG_MTOS_BUFFER_SIZE)&(0xFFFFFF))+CONFIG_MTOS_BUFFER_LEGACY)
#define MTOS_BUFFER_AVAILABLE ((CONFIG_MTOS_BUFFER_SIZE)&(0xFFFFFF))
#define MTOS_BUFFER_SLAVE (2*CONFIG_MTOS_BUFFER_LEGACY)
#define MTOS_EVT_POST(x,y,z) esp_event_post_to(mtos_loop_handle,MTOS_EVENTS,x,y,z,CONFIG_MTOS_UART_STEP_MS/portTICK_PERIOD_MS)

static QueueHandle_t mtos_call_queue;
static QueueHandle_t mtos_uart_queue;
static esp_event_loop_handle_t mtos_loop_handle;
static int uart_master_timeout = CONFIG_MTOS_DEFAULT_TIMEOUT;
static int uart_slave_timeout = CONFIG_MTOS_DEFAULT_TIMEOUT;
static unsigned int chunk_current_max = MTOS_BUFFER_AVAILABLE;
static uint32_t to;

ESP_EVENT_DEFINE_BASE(MTOS_EVENTS);

int mtos_call(char* name, unsigned int timeout_ms, unsigned int max_chunk_size)
{
    mtos_list_t* node = mtos_lookup(name);
    if (node != NULL) {
        if (!node->slave) {
            ESP_LOGI(TAG,"found %s",node->name);
            xQueueSend(mtos_call_queue,&node,portMAX_DELAY);
            uart_master_timeout = timeout_ms;
            if ((max_chunk_size <= MTOS_BUFFER_AVAILABLE) && (max_chunk_size >= CONFIG_MTOS_BUFFER_LEGACY)) {
                chunk_current_max = max_chunk_size;
            }
            else if (max_chunk_size < CONFIG_MTOS_BUFFER_LEGACY) {
                chunk_current_max = CONFIG_MTOS_BUFFER_LEGACY;
            }
            else {
                chunk_current_max = MTOS_BUFFER_AVAILABLE;
            }
            return 0;
        }
        else {
            return -2;
        }
    }
    else {
        return -1;
    }
}

static void mtos_main_uart(void* pvParameters)
{
    char *TAG = "mtos_uart";
    mtos_uart_vessel_t uart_call = {};
    int uart_result = 0;
    size_t last_rx_bytes = 0;
    size_t buffered_size = 0;
    for(;;) {
        uint32_t last_rx_time = MILLIS(0);
        uint32_t delta_ms = MILLIS(0);
        while(1) {
            last_rx_bytes = buffered_size;
            vTaskDelay(10/portTICK_RATE_MS);
            uart_get_buffered_data_len(CONFIG_MTOS_UART_PORT, &buffered_size);
            if (buffered_size != last_rx_bytes) {
                last_rx_time = MILLIS(0);
            }
            else {
                if(delta_ms > CONFIG_MTOS_UART_STEP_MS) {
                    break;
                }
                else {
                    delta_ms = MILLIS(last_rx_time);
                }
            }
        }
        ESP_LOGD("ultradbg","messages waiting now: %u",uxQueueMessagesWaiting(mtos_uart_queue));
        ESP_LOGD("ultradbg","[[[[[[[[RECIEVING QUEUE FROM read_bytes > uart_main]]]]]]]]");
        if (xQueueReceive(mtos_uart_queue,&uart_call,2*CONFIG_MTOS_UART_STEP_MS/portTICK_PERIOD_MS) == pdTRUE) {
            last_rx_bytes = *uart_call.rx_bytes;
            uart_result = uart_read_bytes(CONFIG_MTOS_UART_PORT, uart_call.buffer+*uart_call.rx_bytes, buffered_size, 0);
            *uart_call.rx_bytes += (uart_result > 0 ? uart_result : 0);
            if (*uart_call.rx_bytes != last_rx_bytes) {
                ESP_LOGI(TAG,"rx_bytes pre: %u >>>",last_rx_bytes);
                ESP_LOG_BUFFER_HEXDUMP(TAG,uart_call.buffer,*uart_call.rx_bytes,ESP_LOG_INFO);
                ESP_LOGI(TAG,"rx_bytes pos: %u <<<",*uart_call.rx_bytes);
            }
            xTaskNotifyGive(uart_call.th);
            uart_call.th = xTaskGetCurrentTaskHandle();
            do {
                ESP_LOGD("ultradbg","!!messages waiting now: %u",uxQueueMessagesWaiting(mtos_uart_queue));
                ESP_LOGD("ultradbg","]]]]]]]]QUEUE SENDIG FROM  uart_main > read_bytes[[[[[[[[");
            } while (xQueueSend(mtos_uart_queue,&uart_call,CONFIG_MTOS_UART_STEP_MS/portTICK_PERIOD_MS) != pdTRUE);
            ulTaskNotifyTake(pdTRUE,2*CONFIG_MTOS_UART_STEP_MS/portTICK_PERIOD_MS);
            vTaskDelay(1/portTICK_RATE_MS);
        }
    }
}


static size_t mtos_read_bytes(void *buf, size_t *length, void *ptr)
{
    ESP_LOGD("ultradbg","/entering mtos_read_bytes\\");
    mtos_uart_vessel_t input = {
        .buffer = buf,
        .rx_bytes = length,
        .header = ptr,
        .th = xTaskGetCurrentTaskHandle()
    };
    if (uxQueueMessagesWaiting(mtos_uart_queue)) {
        ESP_LOGD("ultradbg","retained messages: %u",uxQueueMessagesWaiting(mtos_uart_queue));
        mtos_uart_vessel_t retained = {};
        if (xQueueReceive(mtos_uart_queue,&retained,
            CONFIG_MTOS_UART_STEP_MS/portTICK_PERIOD_MS) == pdTRUE) {
            ESP_LOGD("ultradbg","retained vessel recieved");
            if (retained.th != xTaskGetCurrentTaskHandle()) {
                ESP_LOGD("ultradbg","retained vessel from uart_main (%u bytes)",*retained.rx_bytes);
                ESP_LOG_BUFFER_HEXDUMP("ultradbg",retained.buffer,*retained.rx_bytes,ESP_LOG_DEBUG);
                ESP_LOGD("ultradbg","now retained messages: %u",uxQueueMessagesWaiting(mtos_uart_queue));
                return *retained.rx_bytes;
            }
            else {
                ESP_LOGD("ultradbg","retained vessel not for this task");
                return *input.rx_bytes;
            }
        }
        else {
            ESP_LOGD("ultradbg","retained vessel not recieved");
            return *input.rx_bytes;
        }
    }
    else {
        mtos_uart_vessel_t new = {};
        ESP_LOGD("ultradbg","[[[[[[[[QUEUE SENDIG FROM read_bytes > uart_main]]]]]]]]");
        if (xQueueSend(mtos_uart_queue,&input,CONFIG_MTOS_UART_STEP_MS/portTICK_PERIOD_MS) == pdTRUE) {
            ESP_LOGD("ultradbg","/vessel sended\\");
            if (ulTaskNotifyTake(pdTRUE,2*CONFIG_MTOS_UART_STEP_MS/portTICK_PERIOD_MS) == 0) return *input.rx_bytes; // espero a uart_main haya mandado al queue
            ESP_LOGD("ultradbg","/notif recieved\\");
            ESP_LOGD("ultradbg","messages waiting now: %u",uxQueueMessagesWaiting(mtos_uart_queue));
            ESP_LOGD("ultradbg","[[[[[[[[RECIEVING QUEUE FROM uart_main > read_bytes]]]]]]]]");
            if (xQueueReceive(mtos_uart_queue,&new,2*CONFIG_MTOS_UART_STEP_MS/portTICK_PERIOD_MS) == pdTRUE) {
                xTaskNotifyGive(new.th); // desbloqueo uart_main luego de haber tomado el queue
                ESP_LOGD("ultradbg","/uart_main restored\\");
                if (new.th == input.th) {
                    ESP_LOGD("ultradbg","llego un vessel que no es el que corresponde\n");
                    ESP_LOGD("ultradbg","/abort: returning input\\");
                    return *input.rx_bytes;
                }
                else {
                    ESP_LOGD("ultradbg","llego un vessel correspondiente\n");
                }
                ESP_LOGD("ultradbg","/vessel recieved back\\");
                ESP_LOGD("ultradbg","messages waiting now: %u",uxQueueMessagesWaiting(mtos_uart_queue));
            }
            else {
                ESP_LOGD("ultradbg","/abort: no vessel recieved\\");
                return *input.rx_bytes;
            }
            return *new.rx_bytes;
        }
        return *input.rx_bytes;
    }
}

static void mtos_send_bytes(char* token, mtos_header_t* header, void* chunk, size_t len)
{
    if (token) uart_write_bytes(MTOS_PORT,token,strlen(token));
    if (header) uart_write_bytes(MTOS_PORT,header->raw,sizeof(((mtos_header_t*)0)->raw));
    if (chunk) {
        mtos_crc32_t block_crc = {};
        block_crc.value = crc32_be(0,chunk,len);
        ESP_LOGI(TAG,"sending chunk_response:{.chunk_size:%u,.crc8:%x,.block_crc32:%x}",
            header->chunk_response.size,
            header->chunk_response.crc8,
            block_crc.value);
        uart_write_bytes(MTOS_PORT,chunk,len);
        uart_write_bytes(MTOS_PORT,block_crc.raw,sizeof(mtos_crc32_t));
    }
}

static void mtos_slave_task(void* pvParameters)
{
    char *TAG = "mtos_slave";
    uint8_t *buffer = (uint8_t*)malloc(MTOS_BUFFER_SLAVE); // reservar bloque de datos donde se recibiran los comandos del maestro
    uint8_t *ptr = buffer; // puntero de posicion
    TaskHandle_t master_task_handle = (TaskHandle_t)pvParameters;
    size_t rx_bytes = 0; // cantidad de bytes recibidos por uart
    mtos_list_t* node = NULL; // puntero donde se cargara el nodo a procesar
    size_t chunk_limit = MTOS_BUFFER_AVAILABLE;
    size_t chunk_max = chunk_limit; // max chunk size
    mtos_header_t current_session = {};
    mtos_header_t response = {};
    size_t bytes_confirmed = 0;
    size_t bytes_to_send = 0;
    mtos_slave_status_t status = MTOS_SLAVE_IDLE;
    assert(buffer);
    MTOS_EVT_POST(MTOS_EVENT_SLAVE_INITED,NULL,0);
    for(;;) {
        // if timeout abort
        if ((MILLIS(to) > uart_slave_timeout)&&(status != MTOS_SLAVE_IDLE)) {
            ESP_LOGI(TAG,"timeout expired");
            status = MTOS_SLAVE_ABORT;
            MTOS_EVT_POST(MTOS_EVENT_SLAVE_TIMEOUT,(node?node->name:NULL),(node?sizeof(((mtos_list_t*)0)->name):0));
        }
        // si el puntero ptr avanzo
        if (buffer < ptr) {
            ESP_LOGI(TAG,"buffer < ptr, se elimina %u bytes procesados",ptr-buffer);
            ESP_LOG_BUFFER_HEXDUMP(TAG,buffer,ptr-buffer,ESP_LOG_DEBUG);
            memmove(buffer,ptr,MTOS_BUFFER_SLAVE-(ptr-buffer));
            rx_bytes -= (ptr-buffer); // se descartan los bytes usados
        }
        else if (rx_bytes > CONFIG_MTOS_BUFFER_LEGACY) {
            ESP_LOGI(TAG,"rx_bytes > CONFIG_MTOS_BUFFER_LEGACY, se eliminan bytes sin informacion");
            // si no se extrajo ningun dato y rx_bytes se hacerca al final del buffer
            // se transfiere los bytes mas recientes al comienzo del buffer
            memmove(buffer,buffer+(rx_bytes-CONFIG_MTOS_BUFFER_LEGACY),CONFIG_MTOS_BUFFER_LEGACY);
            rx_bytes = CONFIG_MTOS_BUFFER_LEGACY;
        }

        ptr = buffer;

        // se leen mas datos de uart para que esten disponibles en el proximo ciclo
        ulTaskNotifyTake(pdTRUE,0);
        mtos_read_bytes(buffer,&rx_bytes,ptr);
        xTaskNotifyGive(master_task_handle);

        if (rx_bytes > sizeof(mtos_header_t)) {
            ESP_LOGI(TAG,"suficientes bytes para analizar");
            switch (status) {
                case MTOS_SLAVE_IDLE: {
                    ESP_LOGI(TAG,"MTOS_SLAVE_IDLE");
                    node = mtos_list_head;
                    status = MTOS_SLAVE_ABORT;
                    ESP_LOGI(TAG,"buscando nodo");
                    do {
                        ESP_LOGI(TAG,"node: %p | trigger: %s",node,node->trigger);
                        ESP_LOG_BUFFER_HEXDUMP(TAG,buffer,rx_bytes,ESP_LOG_DEBUG);
                        if (node->slave) {
                            ptr = memmem(buffer,rx_bytes,node->trigger,strlen(node->trigger));
                            if (ptr != NULL) {
                                ESP_LOGI(TAG,"trigger found!");
                                memcpy(&current_session,ptr+strlen(node->pattern),sizeof(mtos_header_t));
                                ESP_LOGI(TAG,"recieved crc8: %02X",current_session.chunk_request.crc8);
                                ESP_LOGI(TAG,"raw: %02X %02X %02X %02X",current_session.raw[0],current_session.raw[1],current_session.raw[2],current_session.raw[3]);
                                if (crc8_be(0,current_session.raw,sizeof(mtos_header_t)-1)
                                == current_session.chunk_request.crc8) {
                                    ESP_LOGI(TAG,"nodo encontrado");
                                    to = MILLIS(0);
                                    MTOS_EVT_POST(MTOS_EVENT_SLAVE_DEMANDED,node->name,sizeof(((mtos_list_t*)0)->name));
                                    ESP_LOGI(TAG,"timeout reset");
                                    ESP_LOGI(TAG,"recieved trigger:{.max_size:%u,.resend:%u.crc8:%x}",
                                    current_session.chunk_request.max_size,
                                    current_session.chunk_request.resend,
                                    current_session.chunk_request.crc8);
                                    chunk_max = (current_session.chunk_request.max_size < chunk_limit ?
                                        current_session.chunk_request.max_size : chunk_limit);
                                    if (current_session.chunk_request.resend) {
                                        ESP_LOGI(TAG,"error: trigger con comando de resend => MTOS_SLAVE_ABORT");
                                    }
                                    else {
                                        ESP_LOGI(TAG,">>>reacomodamiento inicial de buffer");
                                        ESP_LOGI(TAG,"rx_bytes: %u",rx_bytes);
                                        ESP_LOG_BUFFER_HEXDUMP(TAG,buffer,rx_bytes,ESP_LOG_DEBUG);
                                        uint8_t* aux = (uint8_t*)memmem(buffer,rx_bytes,node->trigger,strlen(node->trigger));
                                        if (aux) {
                                            size_t removed_header_length = aux+strlen(node->trigger)-buffer;
                                            memmove(buffer+strlen(node->pattern),buffer+removed_header_length,rx_bytes-removed_header_length);
                                            memcpy(buffer,node->pattern,strlen(node->pattern));
                                            rx_bytes += strlen(node->trigger);
                                            rx_bytes -= removed_header_length;
                                            ESP_LOGI(TAG,"rx_bytes: %u",rx_bytes);
                                            ESP_LOG_BUFFER_HEXDUMP(TAG,buffer,rx_bytes,ESP_LOG_DEBUG);
                                            ESP_LOGI(TAG,"<<<");
                                        }
                                        status = MTOS_SLAVE_INIT;
                                        ptr = buffer;
                                    }
                                    break; // sale de loop con node asignado
                                }
                                else {
                                    ESP_LOGI(TAG,"fallo verif. crc8");
                                }
                            }
                            else {
                                ESP_LOGI(TAG,"trigger not found.");
                            }
                        }
                        node = node->next;
                    } while(node);
                    if (node == NULL) {
                        ESP_LOGI(TAG,"error: nodo no encontrado => MTOS_SLAVE_ABORT");
                        break;
                    }
                }
                case MTOS_SLAVE_INIT: {
                    ESP_LOGI(TAG,"MTOS_SLAVE_INIT");
                    if (status == MTOS_SLAVE_ABORT) {
                        ESP_LOGI(TAG,"falltrough con status en MTOS_SLAVE_ABORT se aborta");
                        break;
                    }
                    if (xSemaphoreTake(node->smphr, (uart_slave_timeout)/portTICK_PERIOD_MS) != pdTRUE) {
                        ESP_LOGI(TAG,"no se pudo tomar el semaforo a tiempo");
                        status = MTOS_SLAVE_ABORT;
                        break;
                    }
                    else {
                        // trigger response
                        ESP_LOGI(TAG,"semaforo tomado (%p), se prosesa la respuesta al trigger",node->smphr);
                        response.trigger_response.payload_length = node->length;
                        response.trigger_response.crc8 = crc8_be(0,response.raw,sizeof(mtos_header_t)-1);
                        ESP_LOGI(TAG,"sending trigger_response:{.payload_length:%u,.crc8:%x}",
                            response.trigger_response.payload_length,
                            response.trigger_response.crc8);
                        mtos_send_bytes(node->trigger,&response,NULL,0);
                        memset(&response,'\0',sizeof(mtos_header_t));
                        status = MTOS_SLAVE_CHUNK;
                    }
                }
                case MTOS_SLAVE_CHUNK: {
                    // chunk response
                    ESP_LOGI(TAG,"MTOS_SLAVE_CHUNK");
                    status = MTOS_SLAVE_ABORT;
                    if (node != NULL) {
                        if (node->slave) {
                            status = MTOS_SLAVE_CHUNK;
                            ptr = memmem(buffer,rx_bytes,node->pattern,strlen(node->pattern));
                            if (ptr != NULL) {
                                ESP_LOGI(TAG,"pattern found!");
                                memcpy(&current_session,ptr+strlen(node->pattern),sizeof(mtos_header_t));
                                ESP_LOGI(TAG,"recieved crc8: %02X",current_session.chunk_request.crc8);
                                ESP_LOGI(TAG,"raw: %02X %02X %02X %02X",current_session.raw[0],current_session.raw[1],current_session.raw[2],current_session.raw[3]);
                                if (crc8_be(0,current_session.raw,sizeof(mtos_header_t)-1)
                                == current_session.chunk_request.crc8) {
                                    to = MILLIS(0);
                                    ESP_LOGI(TAG,"timeout reset");
                                    ESP_LOGI(TAG,"recieved chunk_request:{.max_size:%u,.resend:%u.crc8:%x}",
                                        current_session.chunk_request.max_size,
                                        current_session.chunk_request.resend,
                                        current_session.chunk_request.crc8);
                                    chunk_max = (current_session.chunk_request.max_size < chunk_limit ?
                                        current_session.chunk_request.max_size : chunk_limit);
                                    mtos_event_chunk_t* evt = (mtos_event_chunk_t*)calloc(1,sizeof(mtos_event_chunk_t));
                                    if (evt) {
                                        evt->chunk_rq.max_size = current_session.chunk_request.max_size;
                                        evt->chunk_rq.tx_size = chunk_max;
                                        evt->chunk_rq.resend = current_session.chunk_request.resend;
                                        evt->chunk_rq.name = node->name;
                                        MTOS_EVT_POST(MTOS_EVENT_SLAVE_CHUNK_RQ,evt,sizeof(mtos_event_chunk_t));
                                    }
                                    else {
                                        MTOS_EVT_POST(MTOS_EVENT_SLAVE_ALLOC_ERROR,node->name,sizeof(((mtos_list_t*)0)->name));
                                    }
                                    if (current_session.chunk_request.resend == 0) {
                                        bytes_confirmed += bytes_to_send;
                                        bytes_to_send = bytes_confirmed + chunk_max > node->length ? node->length - bytes_confirmed : chunk_max;
                                        response.chunk_response.count++;
                                        if (bytes_confirmed == node->length) {
                                            MTOS_EVT_POST(MTOS_EVENT_SLAVE_FINISHED,node->name,sizeof(((mtos_list_t*)0)->name));
                                            status = MTOS_SLAVE_ENDING;
                                            break;
                                        }
                                    }
                                    uint8_t* send_ptr = node->ptr;
                                    send_ptr += bytes_confirmed;
                                    response.chunk_response.size = bytes_to_send;
                                    response.chunk_response.crc8 = crc8_be(0,response.raw,sizeof(mtos_header_t)-1);
                                    mtos_send_bytes(node->pattern,&response,send_ptr,bytes_to_send);
                                }
                                ptr += strlen(node->pattern)+sizeof(mtos_header_t);
                            }
                            else {
                                ESP_LOGI(TAG,"pattern not found!");
                            }
                        }
                    }
                    if (status == MTOS_SLAVE_ABORT) {
                        ESP_LOGI(TAG,"error en node seleccionado => MTOS_SLAVE_ABORT");
                    }
                }
                default: break;
            }
        }

        if ((status == MTOS_SLAVE_ABORT)||(status == MTOS_SLAVE_ENDING)) {
            // luego de la finalizacion o el aborto, se reinicia el estado
            ESP_LOGI(TAG,"MTOS_SLAVE_ABORT/MTOS_SLAVE_ENDING");
            if (node) {
                ESP_LOGI(TAG,"smphr: %p",node->smphr);
                xSemaphoreGive(node->smphr);
                ESP_LOGI(TAG,"semaforo liberado");
            }
            MTOS_EVT_POST(MTOS_EVENT_SLAVE_RELEASED,(node?node->name:NULL),(node?sizeof(((mtos_list_t*)0)->name):0));
            rx_bytes = 0;
            ptr = buffer;
            bytes_confirmed = 0;
            bytes_to_send = 0;
            node = NULL;
            status = MTOS_SLAVE_IDLE;
        }
    }
}

static void mtos_master_task(void *pvParameters)
{
    char *TAG = "mtos_master";
    TaskHandle_t slave_task_handle;
    TaskHandle_t master_task_handle = xTaskGetCurrentTaskHandle();
    uint8_t *buffer = (uint8_t*)malloc(MTOS_BUFFER_EFFECTIVE); // reservar bloque de datos donde se recibiran los bloques enviados por uart
    uint8_t *ptr = buffer; // puntero a una posicion dentro del bloque reservado
    uint8_t *acc = NULL;  // acumulador de chunks
    size_t rx_bytes = 0; // cantidad de bytes recibidos por uart
    size_t payload_size = 0; // cantidad de bytes totales a recibir
    size_t payload_count = 0; // canidad de bytes recibidos
    mtos_list_t* node = NULL; // puntero donde se cargara el nodo a procesar
    mtos_master_status_t status = MTOS_MASTER_IDLE; // estado inicial de la maquina de estado
    mtos_header_t extracted = {}; // variable auxiliar donde se volcaran los headers que se vayan recibiendo
    mtos_header_t outgoing = {}; // headers que se utilizan en los mensajes de request
    char* token = NULL; // puntero donde se asigna el string que se desea buscar en el buffer de datos recibidos
    size_t token_len = 0; // largo del string que se desea buscar en el buffer de datos recibidos
    assert(buffer);
    for(;;) {
        xTaskCreate(mtos_slave_task, "mtos_slv", 4096, (void*)master_task_handle, uxTaskPriorityGet(NULL)-1, &slave_task_handle);
        MTOS_EVT_POST(MTOS_EVENT_MASTER_IDLE,(node?node->name:NULL),(node?sizeof(((mtos_list_t*)0)->name):0));
        xQueueReceive(mtos_call_queue,&node,portMAX_DELAY);
        MTOS_EVT_POST(MTOS_EVENT_MASTER_CALL,node->name,sizeof(((mtos_list_t*)0)->name));
        ESP_LOGI(TAG,"node recibido por queue");
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        vTaskDelete(slave_task_handle); //porque no puede recibir un bloque mientras esta enviando otro
        to = MILLIS(0);
        ESP_LOGI(TAG,"timeout reset");
        xSemaphoreTake(node->smphr, (uart_master_timeout+10)/portTICK_PERIOD_MS);
        ESP_LOGI(TAG,"node smphr taken");
        for(;;) {
            // if timeout abort
            if (MILLIS(to) > uart_master_timeout) {
                ESP_LOGI(TAG,"timeout expired");
                status = MTOS_MASTER_ABORT;
                MTOS_EVT_POST(MTOS_EVENT_MASTER_TIMEOUT,node->name,sizeof(((mtos_list_t*)0)->name));
            }
            // si el puntero ptr avanzo
            if (buffer < ptr) {
                ESP_LOGI(TAG,"buffer < ptr, se elimina %u bytes procesados",ptr-buffer);
                ESP_LOG_BUFFER_HEXDUMP(TAG,buffer,ptr-buffer,ESP_LOG_DEBUG);
                // se transfieren los datos que faltan analizar, a partir de ptr, hacia el comienzo del buffer
                memmove(buffer,ptr,MTOS_BUFFER_EFFECTIVE-(ptr-buffer));
                rx_bytes -= (ptr-buffer); // se descartan los bytes usados
            }
            /* else if (rx_bytes > CONFIG_MTOS_BUFFER_LEGACY) {
                ESP_LOGI(TAG,"rx_bytes > CONFIG_MTOS_BUFFER_LEGACY, se eliminan bytes sin informacion");
                // si no se extrajo ningun dato y rx_bytes se hacerca al final del buffer
                // se transfiere los bytes mas recientes al comienzo del buffer
                memmove(buffer,buffer+(rx_bytes-CONFIG_MTOS_BUFFER_LEGACY),CONFIG_MTOS_BUFFER_LEGACY);
                rx_bytes = CONFIG_MTOS_BUFFER_LEGACY;
            }*/

            ptr = buffer;

            // se leen mas datos de uart para que esten disponibles en el proximo ciclo
            mtos_read_bytes(buffer,&rx_bytes,ptr);

            if ((rx_bytes >= token_len+sizeof(mtos_header_t)) && (extracted.uint32 == 0) && (token != NULL)) {
                // cuando se recibieron suficientes bytes por uart para extraer un header
                // busco el string alojado en token en el buffer de datos recibidos
                ESP_LOGI(TAG,"suficientes bytes recibidos para procesar, token asignado: %.*s",token_len,token);
                ptr = memmem(buffer,rx_bytes,token,token_len);
                if (ptr != NULL) {
                    ESP_LOGI(TAG,"token hallado");
                    // restablecimiento de contador timeout
                    to = MILLIS(0);
                    ESP_LOGI(TAG,"timeout reset");
                    // al encontrarlo avanzo el puntero hacia el primer byte luego del token encontrado
                    ptr += token_len;
                    // como se trata de un header, se copia a la variable correspondiente
                    memcpy(&extracted,ptr,sizeof(mtos_header_t));
                    ptr += sizeof(mtos_header_t);
                }
                else {
                    ESP_LOGI(TAG,"Token no encontrado en %u bytes",rx_bytes);
                    ESP_LOG_BUFFER_HEXDUMP(TAG,buffer,rx_bytes,ESP_LOG_DEBUG);
                }
            }

            // maquina de estados
            switch (status) {
                case MTOS_MASTER_ABORT: {
                    ESP_LOGI(TAG,"MTOS_MASTER_ABORT inicial");
                    // se descartan los datos recibidos hasta ahora
                    // se reinician las variables al estado inicial
                    if (acc) {
                        free(acc);
                        acc = NULL;
                    }
                    ptr = buffer+rx_bytes;
                    payload_size = 0;
                    payload_count = 0;
                    token = NULL;
                    token_len = 0;
                    break;
                }
                case MTOS_MASTER_IDLE: {
                    ESP_LOGI(TAG,"MTOS_MASTER_IDLE");
                    // se envia el string almacenado en trigger
                    // esto indica al equipo remoto que comience la transferencia de datos
                    outgoing.chunk_request.max_size = chunk_current_max;
                    outgoing.chunk_request.resend = false;
                    outgoing.chunk_request.crc8 = crc8_be(0,outgoing.raw,sizeof(mtos_header_t)-1);
                    ESP_LOGI(TAG,"sending trigger:{.max_size:%u,.resend:%u,.crc8:%x}",
                    outgoing.chunk_request.max_size,
                    outgoing.chunk_request.resend,
                    outgoing.chunk_request.crc8);
                    ESP_LOGI(TAG,"raw: %02X %02X %02X %02X",outgoing.raw[0],outgoing.raw[1],outgoing.raw[2],outgoing.raw[3]);
                    mtos_send_bytes(node->trigger,&outgoing,NULL,0);
                    status++;
                    break;
                }
                case MTOS_MASTER_INIT: {
                    if (token == NULL) ESP_LOGI(TAG,"MTOS_MASTER_INIT");
                    // en esta fase se espera la respuesta al envio del trigger enviado en el estado anterior
                    if (extracted.uint32) {
                        // se pudo hallar un header en los datos recibidos por uart
                        // se verifica la integridad del header recibido
                        if (crc8_be(0,extracted.raw,sizeof(mtos_header_t)-1)
                             == extracted.trigger_response.crc8) {
                                ESP_LOGI(TAG,"crc8 verificado ok");
                                ESP_LOGI(TAG,"recieved trigger_response:{.payload_length:%u,.crc8:%x}",
                                extracted.trigger_response.payload_length,
                                extracted.trigger_response.crc8);
                            // crc verificado ok
                            // como se trata de la respuesta al trigger se reserva el bloque de memoria para el acumulador
                            acc = (uint8_t*)malloc(extracted.trigger_response.payload_length);
                            if (acc) {
                                // se pudo reservar el bloque donde se iran acumulando los bytes recibidos
                                ESP_LOGI(TAG,"%u bytes alocados",extracted.trigger_response.payload_length);
                                payload_size = extracted.trigger_response.payload_length;
                                payload_count = 0;
                                extracted.uint32 = 0;
                                status++;
                                MTOS_EVT_POST(MTOS_EVENT_MASTER_ANSWERED,node->name,sizeof(((mtos_list_t*)0)->name));
                            }
                        }
                        if (!acc) {
                            ESP_LOGI(TAG,"error, fallback por alocacion fallida");
                            // no se pudo asignar los bytes necesarios
                            // o no se pudo verificar el crc del header
                            status--;
                            MTOS_EVT_POST(MTOS_EVENT_MASTER_ALLOC_ERROR,node->name,sizeof(((mtos_list_t*)0)->name));
                            break;
                        }
                    }
                    else {
                        // no se ha detectado la respuesta al trigger
                        // se asigna a token el patron a detectar en la respuesta esperada
                        if ((token ? strcmp(token,node->trigger) : false)) ESP_LOGI(TAG,"setup para token: %.*s",token_len,token);
                        token = node->trigger;
                        token_len = strlen(node->trigger);
                        break;
                    }
                }
                case MTOS_MASTER_CHUNK: {
                    if (!strcmp(token,node->trigger)) ESP_LOGI(TAG,"MTOS_MASTER_CHUNK");
                    // fase de recepcion de chunks
                    if (extracted.uint32) {
                        // se verifica la integridad del header recibido
                        if (crc8_be(0,extracted.raw,sizeof(mtos_header_t)-1)
                             == extracted.chunk_response.crc8) {
                                ESP_LOGI(TAG,"crc8 verificado ok");
                                // crc verificado ok
                                // el header de un chunk contiene el tamaño de la porcion del bloque que se envio
                                // se verifica que la cantidad de bytes recibidos por uart sea la sufuiciente para
                                // albergar la cantidad de bytes que indica el header
                            if (extracted.chunk_response.size+sizeof(mtos_crc32_t) <= rx_bytes) {
                                // cantidad de bytes recibidos es suficiente
                                ESP_LOGI(TAG,"suficiente cantidad de bytes para procesar");
                                mtos_chunk_vessel_t new = {
                                    .chunk = ptr,
                                    .size = extracted.chunk_response.size,
                                };
                                // se verifica el crc32 de los bytes del chunk sin contar el header
                                memcpy(new.crc32.raw,new.chunk+new.size,sizeof(mtos_crc32_t));
                                ESP_LOGI(TAG,"recieved chunk_response:{.size:%u,.count:%u,.crc8:%x,.payload_crc32:%x}",
                                extracted.chunk_response.size,
                                extracted.chunk_response.count,
                                extracted.chunk_response.crc8,
                                new.crc32.value);
                                if (crc32_be(0,new.chunk,new.size) == new.crc32.value) {
                                    // la verificacion  es correcta se agregan los bytes al acumulador
                                    memcpy(acc+payload_count,new.chunk,new.size);
                                    mtos_event_chunk_t* evt = (mtos_event_chunk_t*)calloc(1,sizeof(mtos_event_chunk_t));
                                    if (evt) {
                                        evt->chunk_rx.chunk = acc+payload_count;
                                        evt->chunk_rx.size = new.size;
                                        evt->chunk_rx.name = node->name;
                                        payload_count += new.size;
                                        evt->chunk_rx.count = payload_count;
                                        evt->chunk_rx.pending = payload_size - payload_count;
                                        MTOS_EVT_POST(MTOS_EVENT_MASTER_CHUNK_RX,evt,sizeof(mtos_event_chunk_t));
                                    }
                                    else {
                                        payload_count += new.size;
                                        MTOS_EVT_POST(MTOS_EVENT_MASTER_ALLOC_ERROR,node->name,sizeof(((mtos_list_t*)0)->name));
                                    }
                                    ptr += new.size+sizeof(mtos_crc32_t); // se adelanta el puntero
                                    outgoing.chunk_request.resend = false;
                                    ESP_LOGI(TAG,"payload_size: %u | payload_count: %u",payload_size,payload_count);
                                    if (payload_count >= payload_size) {
                                        // ya se recibio la totalidad de bytes del payload
                                        ESP_LOGI(TAG,"ya se recibio la totalidad de bytes del payload => MTOS_MASTER_ENDING");
                                        status++;
                                    }
                                }
                                else {
                                    ESP_LOGI(TAG,"validacion de crc32 del bloque fallida");
                                    // no se verifico correctamente crc32
                                    // se solicita retransmision
                                    outgoing.chunk_request.resend = true;
                                }
                            }
                            else {
                                ESP_LOGI(TAG,"insuficiente cantidad de bytes para procesar");
                                ESP_LOGI(TAG,"extracted.chunk_response.size+sizeof(mtos_crc32_t) [%u] <= rx_bytes [%u]",
                                extracted.chunk_response.size+sizeof(mtos_crc32_t),rx_bytes);
                                // en caso que no haya suficientes datos recibidos por uart
                                // se debe preservar el header para el siguiente ciclo
                                // se setea el byte de resend en un valor especifico
                                // que luego permitira saltar la transmicion de un chunk_request
                                outgoing.chunk_request.resend = 0xFF;
                            }
                        }
                        if (outgoing.chunk_request.resend != 0xFF) {
                            // enviar solicitud de chunk
                            // la maxima cantidad de bytes que puede recibir en el proximo chunk
                            outgoing.chunk_request.crc8 = crc8_be(0,outgoing.raw,sizeof(mtos_header_t)-1);
                            ESP_LOGI(TAG,"sending chunk_request:{.max_size:%u,.resend:%u,.crc8:%x}",
                            outgoing.chunk_request.max_size,
                            outgoing.chunk_request.resend,
                            outgoing.chunk_request.crc8);
                            mtos_send_bytes(node->pattern,&outgoing,NULL,0);
                            extracted.uint32 = 0;
                        }
                    }
                    else {
                        // no se ha detectado la respuesta al pattern
                        // se asigna a token el patron a detectar en la respuesta esperada
                        
                        if (token ? strcmp(token,node->pattern) : false) ESP_LOGI(TAG,"setup para token: %.*s",token_len,token);
                        token = node->pattern;
                        token_len = strlen(node->pattern);
                    }
                    break;
                }
                case MTOS_MASTER_ENDING: {
                    ESP_LOGI(TAG,"MTOS_MASTER_ENDING inicial");
                    // librar la memoria del miembro ptr del nodo
                    free(node->ptr);
                    // asignarle el puntero donde se estuvieron acumulando los datos
                    node->ptr = acc;
                    // tambien se actualiza el miembro length del nodo
                    node->length = payload_size;
                    // calcular el nuevo crc
                    node->crc32.value = crc32_be(0,node->ptr,node->length);
                    // enviar evento
                    MTOS_EVT_POST(MTOS_EVENT_MASTER_UPDATED,node->name,sizeof(((mtos_list_t*)0)->name));
                    // reiniciar variables
                    ptr = buffer+rx_bytes;
                    acc = NULL;
                    payload_size = 0;
                    payload_count = 0;
                    token = NULL;
                    token_len = 0;
                    break;
                }
            }

            if ((status == MTOS_MASTER_ABORT)||(status == MTOS_MASTER_ENDING)) {
                ESP_LOGI(TAG,"MTOS_MASTER_ABORT/MTOS_MASTER_ENDING final");
                if (acc == NULL) {
                    ESP_LOGI(TAG,"cierre habilitado por acc = NULL");
                    // luego de la finalizacion o el aborto, se reinicia el estado
                    status = MTOS_MASTER_IDLE;
                    // se sale de la maquina de estado para esperar el proximo elemento de la cola
                    break;
                }
            }
        }
        xSemaphoreGive(node->smphr);
    }
}

mtos_event_handler_t mtos_usr_cb = NULL;

static void mtos_cb_handler_intern(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    if (mtos_usr_cb) {
        mtos_usr_cb(id,event_data,handler_args);
    }
}

void mtos_init(mtos_event_handler_t evt_callback, void* usr_data) {
    mtos_usr_cb = evt_callback;
    esp_event_loop_args_t mtos_loop_args = {
        .queue_size = CONFIG_MTOS_EVT_QUEUE_SIZE,
        .task_name = "mtos_evt_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&mtos_loop_args, &mtos_loop_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(mtos_loop_handle, MTOS_EVENTS, ESP_EVENT_ANY_ID, mtos_cb_handler_intern, usr_data, NULL)); //puntero a los datos

    mtos_call_queue = xQueueCreate(CONFIG_MTOS_CALL_QUEUE_LENGTH,sizeof(mtos_list_t*));
    mtos_uart_queue = xQueueCreate(CONFIG_MTOS_CALL_QUEUE_LENGTH,sizeof(mtos_uart_vessel_t));
    uart_config_t uart_config = {
        .baud_rate = CONFIG_MTOS_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(MTOS_PORT, MTOS_BUFFER_EFFECTIVE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MTOS_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(MTOS_PORT, CONFIG_MTOS_UART_TX_PIN, CONFIG_MTOS_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    xTaskCreate(mtos_master_task, "mtos_mst", 4096, NULL, uxTaskPriorityGet(NULL), NULL);
    xTaskCreate(mtos_main_uart, "mtos_uart",4096, NULL, uxTaskPriorityGet(NULL), NULL);
}