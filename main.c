/**
 * Transmissor - Lê temperatura e umidade do sensor AHT10.
 *
 * Este arquivo inicializa a comunicação I2C, lê os dados do sensor AHT10,
 * e imprime os valores de temperatura, umidade e pressão atmosférica no console,
 * enviando também os dados via UART.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "aht20.h"
#include "bmp280.h"

// ================= UART CONFIG =================
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define UART_BAUDRATE 115200

static void mock_sensor_data(AHT20_Data *dados) {
    static float temp = 24.0f;
    static float hum  = 55.0f;
    static int dir_t  = 1;
    static int dir_h  = 1;

    // Varia temperatura entre 24 e 30
    temp += 0.05f * dir_t;
    if (temp > 30.0f || temp < 24.0f) {
        dir_t = -dir_t;
    }

    // Varia umidade entre 45 e 70
    hum += 0.1f * dir_h;
    if (hum > 70.0f || hum < 45.0f) {
        dir_h = -dir_h;
    }

    dados->temperature = temp;
    dados->humidity    = hum;
}
struct bmp280_calib_param params; 
int main() {
    stdio_init_all();

    // printf("Iniciando leitura da temperatura e humidade através do sensor AHT10...\n");
    // UART INIT
    uart_init(UART_ID, UART_BAUDRATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, true);

    // I2C INIT sensor
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    // Inicializa o BMP280
    bmp280_init(I2C_PORT);
    bmp280_get_calib_params(I2C_PORT, &params);

    // Inicializa o AHT20
    aht20_reset(I2C_PORT);
    aht20_init(I2C_PORT);

    char uart_buffer[64];
    int32_t raw_temp_bmp;
    int32_t raw_pressure;
    char str_tmp[8];  // Buffer maior para armazenar a string
    char str_press[8];  // Buffer maior para armazenar a string  
    char str_umi[8];  // Buffer maior para armazenar a string 
    AHT20_Data data;

    while (1) {
        // Leitura do BMP280
        bmp280_read_raw(I2C_PORT, &raw_temp_bmp, &raw_pressure);
        int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params);

        // printf("Pressao = %.3f kPa\n", pressure / 1000.0);
        // printf("Temperatura BMP: = %.2f C\n", temperature / 100.0);

        // Leitura do AHT20
        if (aht20_read(I2C_PORT, &data)){
            // printf("Temperatura AHT: %.2f C\n", data.temperature);
            // printf("Umidade: %.2f %%\n", data.humidity);
        } else {
            printf("Erro na leitura do AHT20!\n");
            // Use valores padrão em caso de erro
            data.temperature = 0.0f;
            data.humidity = 00.0f;
        }   
        
        // Calcule a média da temperatura entre os dois sensores
        float avg_temp = (temperature / 100.0f + data.temperature) / 2.0f;
        // printf("Temperatura média: %.2f C\n", avg_temp);

        snprintf(
            uart_buffer,
            sizeof(uart_buffer),
            "TEMP:%.2f;HUM:%.2f\n",
            avg_temp,
            data.humidity
        );
        printf(uart_buffer);
        uart_puts(UART_ID, uart_buffer);
        sleep_ms(1000);
    }
}
