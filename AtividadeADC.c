/**
 * Controle de LED RGB e Display OLED com Joystick
 * Desenvolvido para Raspberry Pi Pico
 * Este programa permite controlar LEDs RGB e um display OLED usando um joystick
 */

// ======= Includes =======
#include <stdio.h>              // Biblioteca padrão de entrada/saída
#include "pico/stdlib.h"        // Biblioteca principal do Raspberry Pi Pico
#include "hardware/adc.h"       // Biblioteca para controle do ADC
#include "hardware/pwm.h"       // Biblioteca para controle do PWM
#include "hardware/i2c.h"       // Biblioteca para comunicação I2C
#include "inc/ssd1306.h"        // Biblioteca do display OLED
#include "inc/font.h"           // Biblioteca de fontes para o display

// ======= Definições de Pinos =======
// Pinos do Joystick
#define VRX_PIN 26             // Pino analógico X do joystick (movimento horizontal)
#define VRY_PIN 27             // Pino analógico Y do joystick (movimento vertical)
#define SW_PIN 22              // Pino do botão do joystick (pressionar)

// Pinos dos Botões e LEDs
#define BUTTON_A_PIN 5         // Pino do botão adicional
#define LED_R_PIN 13           // Pino do LED vermelho (PWM)
#define LED_G_PIN 11           // Pino do LED verde (digital)
#define LED_B_PIN 12           // Pino do LED azul (PWM)

// Configuração da comunicação I2C
#define I2C_PORT i2c1          // Porta I2C utilizada
#define I2C_SDA 14             // Pino de dados I2C
#define I2C_SCL 15             // Pino de clock I2C
#define ENDERECO 0x3C          // Endereço I2C do display OLED

// ======= Variáveis Globais =======
ssd1306_t ssd;                 // Estrutura de controle do display OLED
int square_x = 60;             // Posição inicial X do quadrado no display
int square_y = 28;             // Posição inicial Y do quadrado no display
bool led_green_state = false;  // Estado do LED verde
bool pwm_enabled = true;       // Estado do PWM
uint8_t border_style = 0;      // Estilo da borda (0-2)

// ======= Constantes =======
#define JOYSTICK_CENTER 2048  // Valor central do ADC (4095/2) - Posição de repouso do joystick
#define ADC_MAX 4095         // Valor máximo do ADC de 12 bits (2^12 - 1)
#define PWM_MAX 65535       // Valor máximo do PWM de 16 bits (2^16 - 1)

// ======= Funções de Configuração PWM =======
void init_pwm(uint gpio) {
    // Configura o pino para função PWM
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    // Define o valor máximo do ciclo PWM
    pwm_set_wrap(slice_num, PWM_MAX);
    // Ativa o PWM
    pwm_set_enabled(slice_num, true);
}

void set_pwm_duty(uint gpio, uint16_t duty) {
    // Define o duty cycle do PWM, considerando se está habilitado
    pwm_set_gpio_level(gpio, pwm_enabled ? duty : 0);
}

// ======= Manipulador de Interrupções =======
void gpio_callback(uint gpio, uint32_t events) {
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = time_us_32();
    
    // Implementa debounce de 200ms
    if (interrupt_time - last_interrupt_time > 200000) {
        if (gpio == SW_PIN) {
            // Alterna estado do LED verde e estilo da borda
            led_green_state = !led_green_state;
            gpio_put(LED_G_PIN, led_green_state);
            border_style = (border_style + 1) % 3;
        } else if (gpio == BUTTON_A_PIN) {
            // Alterna estado do PWM
            pwm_enabled = !pwm_enabled;
        }
        last_interrupt_time = interrupt_time;
    }
}

// ======= Funções de Display =======
void draw_border(ssd1306_t *ssd, uint8_t style) {
    // Desenha diferentes estilos de borda no display
    switch(style) {
        case 0: // Borda simples
            ssd1306_rect(ssd, 0, 0, WIDTH, HEIGHT, true, false);
            break;
        case 1: // Borda dupla
            ssd1306_rect(ssd, 0, 0, WIDTH, HEIGHT, true, false);
            ssd1306_rect(ssd, 2, 2, WIDTH-4, HEIGHT-4, true, false);
            break;
        case 2: // Borda com cantos
            // Linhas horizontais
            ssd1306_hline(ssd, 0, 10, 0, true);
            ssd1306_hline(ssd, WIDTH-10, WIDTH-1, 0, true);
            ssd1306_hline(ssd, 0, 10, HEIGHT-1, true);
            ssd1306_hline(ssd, WIDTH-10, WIDTH-1, HEIGHT-1, true);
            // Linhas verticais
            ssd1306_vline(ssd, 0, 0, 10, true);
            ssd1306_vline(ssd, 0, HEIGHT-10, HEIGHT-1, true);
            ssd1306_vline(ssd, WIDTH-1, 0, 10, true);
            ssd1306_vline(ssd, WIDTH-1, HEIGHT-10, HEIGHT-1, true);
            break;
    }
}

// ======= Função Principal =======
int main() {
    // Inicializações básicas
    stdio_init_all();
    
    // Configuração do ADC
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);

    // Configuração das GPIOs
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);

    // Configuração do PWM para os LEDs RGB
    init_pwm(LED_R_PIN);
    init_pwm(LED_B_PIN);

    // Configuração I2C e Display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Loop Principal
    while (true) {
        // Leitura dos valores do Joystick
        adc_select_input(0);
        uint16_t vrx_value = adc_read();
        adc_select_input(1);
        uint16_t vry_value = adc_read();
        
        // Controle dos LEDs RGB baseado na posição do joystick
        uint16_t red_pwm = 0;
        uint16_t blue_pwm = 0;
        
        // Zona morta de 150 para evitar flutuações quando joystick está próximo do centro
        if (abs(vrx_value - JOYSTICK_CENTER) > 150) {
            // Multiplica por 32 para converter range do ADC para PWM
            blue_pwm = abs(vrx_value - JOYSTICK_CENTER) * 32;
        }
        if (abs(vry_value - JOYSTICK_CENTER) > 150) {
            red_pwm = abs(vry_value - JOYSTICK_CENTER) * 32;
        }

        set_pwm_duty(LED_R_PIN, red_pwm);
        set_pwm_duty(LED_B_PIN, blue_pwm);
        
        // Cálculo da nova posição do quadrado baseado no joystick
        // 60 e 28 são posições iniciais, 114 e 50 são limites de movimento
        square_x = 60 + ((vry_value - JOYSTICK_CENTER) * 114) / ADC_MAX;
        square_y = 28 - ((vrx_value - JOYSTICK_CENTER) * 50) / ADC_MAX;

        // Atualização do Display OLED
        ssd1306_fill(&ssd, false);
        // Desenha quadrado 8x8 pixels na posição calculada
        ssd1306_rect(&ssd, square_y, square_x, 8, 8, true, true);
        draw_border(&ssd, border_style);
        ssd1306_send_data(&ssd);

        sleep_ms(20);  // Delay para controle de taxa de atualização
    }

    return 0;
}
