#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define UART_NUM UART_NUM_0  
#define UART_NUM_EXT UART_NUM_1

#define UART_TX_PIN GPIO_NUM_16 
#define UART_RX_PIN GPIO_NUM_17 

#define UART_EXT_TX_PIN GPIO_NUM_8  
#define UART_EXT_RX_PIN GPIO_NUM_9 	

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static const char *TAG = "UART_BRIDGE";

static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    
    uart_config_t uart_ext_config = {
        .baud_rate = 9600,  // Adjust based on your external device
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_EXT, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_EXT, &uart_ext_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_EXT, UART_EXT_TX_PIN, UART_EXT_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void uart_to_ext_task(void *arg)
{
    uint8_t data[RD_BUF_SIZE];
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, RD_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            ESP_LOGI(TAG, "PC->EXT: %.*s", len, data);
            // Forward to external UART
            uart_write_bytes(UART_NUM_EXT, (const char*)data, len);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void ext_to_uart_task(void *arg)
{
    uint8_t data[RD_BUF_SIZE];
    
    while (1) {
        int len = uart_read_bytes(UART_NUM_EXT, data, RD_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            ESP_LOGI(TAG, "EXT->PC: %.*s", len, data);
            uart_write_bytes(UART_NUM, (const char*)data, len);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void echo_task(void *arg)
{
    uint8_t data[RD_BUF_SIZE];
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, RD_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            ESP_LOGI(TAG, "Echo: %.*s", len, data);
            uart_write_bytes(UART_NUM, (const char*)data, len);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting UART Bridge...");
    
    uart_init();
    
    ESP_LOGI(TAG, "UART initialized");
    ESP_LOGI(TAG, "PC UART: 115200 baud");
    ESP_LOGI(TAG, "External UART: 9600 baud, TX: GPIO%d, RX: GPIO%d", UART_EXT_TX_PIN, UART_EXT_RX_PIN);
    
    
    xTaskCreate(uart_to_ext_task, "uart_to_ext", 4096, NULL, 10, NULL);
    xTaskCreate(ext_to_uart_task, "ext_to_uart", 4096, NULL, 10, NULL);
    
    
    ESP_LOGI(TAG, "UART bridge tasks started");
    
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
