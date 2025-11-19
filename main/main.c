#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define UART_PC UART_NUM_0      // USB to PC
#define UART_EXT UART_NUM_1     // External pins

#define BUF_SIZE 1024
static const char *TAG = "UART_BRIDGE";

static void uart_init(void)
{
    uart_config_t pc_uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_PC, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PC, &pc_uart_config));
    
    uart_config_t ext_uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_EXT, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_EXT, &ext_uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_EXT, GPIO_NUM_8, GPIO_NUM_9, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void pc_to_ext_task(void *arg)
{
    uint8_t data[BUF_SIZE];
    
    while (1) {
        int len = uart_read_bytes(UART_PC, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            uart_write_bytes(UART_EXT, (const char*)data, len);
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

static void ext_to_pc_task(void *arg)
{
    uint8_t data[BUF_SIZE];
    
    while (1) {
        int len = uart_read_bytes(UART_EXT, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            uart_write_bytes(UART_PC, (const char*)data, len);
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting UART Bridge - ESP32");
    
    uart_init();
    
    xTaskCreate(pc_to_ext_task, "pc_to_ext", 4096, NULL, 10, NULL);
    xTaskCreate(ext_to_pc_task, "ext_to_pc", 4096, NULL, 10, NULL);
    
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
