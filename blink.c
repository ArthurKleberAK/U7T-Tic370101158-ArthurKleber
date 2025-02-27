#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "blink.pio.h" //arquivo .pio

// DEFINIÇÕES DO PWM:
#define CLOCK_DIV 64.0 
#define TPWM 20000.0    
#define WRAP 39063

// DEFINIÇÕES DO DISPLAY:
#define I2C_PORT i2c1
#define I2C_SDA 14 // GPIO SDA DO DISPLAY
#define I2C_SCL 15 // GPIO SCL DO DISPLAY
#define endereco 0x3C // ENDEREÇO DO DISPLAY
ssd1306_t ssd; // INICIALIZA A ESTRUTURA DO DISPLAY

// DEFINIÇÕES DOS PINOS:
#define MATRIX_PIN 7 // GPIO DE SAÍDA DA MATRIZ DE LED 5x5
#define BUZZER 10 // GPIO BUZZER
#define LED_R 13 // GPIO LED VERMELHO FASE DE AQUECIMENTO
#define LED_B 12 // GPIO LED AZUL  FASE DE INGREDIENTES
#define LED_G 11 // GPIO LED VERDE INIDICA QUE O PROGRAMA ESTA FUNCIONANDO
#define BOTAO_A 05 // GPIO DO BOTÃO A
#define BOTAO_B 06 // GPIO DO BOTÃO A
#define SW 22 // GPIO DO JOYSTICK
#define DEBOUNCE 200 // TEMPO EM MS DO DEBOUCE DE CADA BOTÃO ACIONADO 

//NÚMERO DE LEDS DA MATRIZ
#define NUM_PIXELS 25 

volatile uint32_t ultima_interrup = 0; // PARA ARMAZENAR O ÚLTIMO TEMPO DE CADA INTERRUPÇÃO 
volatile bool timer_expired = false; // FLAG PARA INDICAR SE O TEMPORIZADO EXPIROU 

// DEFINIÇÃO DOS ESTADOS DA MÁQUINA DE ESTADO
typedef enum {
    ESPERA,
    ENCHER_ING1,
    ENCHER_ING2,
    MISTURAR_AQUECER,
    RESULTADO,
    DRENAR,
    FINALIZAR
} Estado;

// FUNÇÃO RESPONSAVEL POR INICIALIZAR O DISPLAY
void init_display(){
    i2c_init(I2C_PORT, 400 * 1000); // INICIALIZAÇÃO DO I2C UTILIZANDO 400KHZ 
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // DEFINE  GPIO COMO I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // DEFINE  GPIO COMO I2C
    gpio_pull_up(I2C_SDA); // DEFINE GPIO SDA COMO PULLUP
    gpio_pull_up(I2C_SCL); // DEFINE GPIO SCL COMO PULLUP
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // INICIALIZAR O DISPLAY
    ssd1306_config(&ssd); // CONFIGURA O DISPLAY
    ssd1306_fill(&ssd, false); // LIMPA O DISPLAY
}


// FUNÇÃO DE CALLBACK DO TEMPORIZADOR 
bool repeating_timer_callback(struct repeating_timer *t) {
    timer_expired = true; // DEFINE A FLAG PARA INDICAR QUE O TEMPORIZADOR EXPIROU 
    return true; //  RETORNA VERDADEIRO PARA CONTINUAR CHAMANDO A FUNÇÃO 
}

// FUNÇÃO RESPONSAVEL POR DESENHAR NA MATRIZ DE LED:
int MAPPER[25] = {
    24, 23, 22, 21, 20,
    15, 16, 17, 18, 19,    // MAPA DE CADA LED NA BITDOGLAB
    14, 13, 12, 11, 10,
    5, 6, 7, 8, 9,
    4, 3, 2, 1, 0};

typedef struct {
    volatile uint8_t r; // RED
    volatile uint8_t g; // GREEN
    volatile uint8_t b; // BLUE
} Cor;

uint32_t matrix_rgb1(Cor cor) {
    return (cor.r << 24) | (cor.b << 16) | (cor.g << 8);
}

void desenho_pio1(Cor *desenho, uint32_t valor_led, PIO pio, uint sm) {
    for (int16_t i = 0; i < NUM_PIXELS; i++) { 
        int led_matrix_location = MAPPER[i];
        valor_led = matrix_rgb1(desenho[led_matrix_location]);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

Cor matriz_f[25] = {
{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
{0, 0, 0}, {0, 0, 0}, {10, 0, 40}, {0, 0, 0}, {0, 0, 0},
{0, 0, 0}, {10, 0, 40}, {0, 0, 40}, {10, 0, 40}, {0, 0, 0},
{10, 0, 40}, {0, 0, 40}, {0, 0, 40}, {0, 0, 40}, {10, 0, 40}
};
Cor matriz_0[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}
};
// Animação do ingrediente 1
Cor matriz_1[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,1,1}
};

Cor matriz_2[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,1,1}, {0,1,1}
};

Cor matriz_3[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,1,1}, {0,1,1}, {0,1,1}
};

Cor matriz_4[25] = 
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};

Cor matriz_5[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};

Cor matriz_6[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
//7
Cor matriz_7[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};

Cor matriz_8[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};

Cor matriz_9[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};

Cor matriz_10[25] = {
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};

Cor matriz_11[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_12[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_13[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {1,0,1}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_14[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_15[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_16[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {1,0,1}, 
{1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_17[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {1,0,1}, {1,0,1}, 
{1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_18[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {1,0,1}, {1,0,1}, {1,0,1}, 
{1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_19[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_20[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, 
{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
Cor matriz_21[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}
};
Cor matriz_22[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}
};
Cor matriz_23[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}
};
Cor matriz_24[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}
};
Cor matriz_25[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}
};
Cor matriz_26[25] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
{0,0,0}, {1,1,6}, {1,1,6}, {1,1,6}, {1,1,6}
};
Cor matriz_27[25] =
{
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {1,1,6}, {1,1,6}, {1,1,6}
};
Cor matriz_28[25] =
{
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {1,1,6}, {1,1,6}
};
Cor matriz_29[25] =
{
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, 
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {1,1,6}
};

// PONTEIRO PARA ARMAZENAR A MATRIZ ATIVA:
Cor* matrizes[29] = {
matriz_1, matriz_2, matriz_3, matriz_4, matriz_5, matriz_6, matriz_7, matriz_8, matriz_9, matriz_10, matriz_11,
matriz_12, matriz_13, matriz_14, matriz_15, matriz_16, matriz_17, matriz_18, matriz_19, matriz_20, 
matriz_21, matriz_22, matriz_23, matriz_24, matriz_25, matriz_26, matriz_27, matriz_28, matriz_29
};

// FUNÇÃO DE TEMPORIZADOR PARA CRIAR UM ATRASO EM CADA ANIMAÇÃO:
void loop_com_timer_nao_bloqueante(PIO pio,  const uint8_t matrizes[][8], uint valor_led) {
    uint64_t tempo_anterior = time_us_64(); // CAPTURA O TEMPO INICIAL
    int j = 0;} 
void loop_com_timer(PIO pio,  const uint8_t matrizes[][8], uint valor_led) {
    uint64_t tempo_anterior = time_us_64(); // CAPTURA O TEMPO INICIAL
    int k = 10;}


// DEFINIÇÕES DO PWM:
typedef struct { // ESTRUTURA PARA ARMAZENAR O ESTADO DO PWM
    uint slice_num;   // NÚMERO DO SLICE PWM ASSOCIADO PINO
    bool pwm_active;  // INDICA SE O PWM ESTÁ ATIVO OU NÃO
} pwm_state_t;

// FUNÇÃO DE NTERRUPÇÃO DO TEMPORIZADOR DO PWM 
bool pwm_timer_callback(repeating_timer_t *rt) {
    pwm_state_t *state = (pwm_state_t *)rt->user_data; // RECRUTA O ESTADO DO PWM ARMAZENADO EM user_data.

    if (state->pwm_active) {          // VERIFICA SE O PWM ESTÁ ATIVO
        pwm_set_enabled(state->slice_num, false); // DESATICA O PWM NO SLICE CORESPONDENTE 
        state->pwm_active = false;    //  ATUALIZA O ESTADO PARA INDICAR QUE O PWM NÃO ESTÁ MAIS ATIVO .
        return false;                 // Para o temporizador, pois o PWM foi desativado.
    }
    return true;                      // Retorna true para continuar o temporizador se o PWM não estava ativo.
}

// Função para controlar o PWM
void pwm_control(uint pin, uint duration_ms) {
    gpio_set_function(pin, GPIO_FUNC_PWM); // Configura o pino especificado para funcionar como saída PWM.  
    uint slice_num = pwm_gpio_to_slice_num(pin); // Obtém o número do slice PWM associado ao pino.
    pwm_set_wrap(slice_num, WRAP);       // Configura o valor de wrap para o slice PWM (define o período do sinal).
    pwm_set_clkdiv(slice_num, CLOCK_DIV); // Configura o divisor de clock para o slice PWM (afeta a frequência).
    pwm_set_enabled(slice_num, true);    // Ativa o PWM no slice especificado.
    pwm_set_chan_level(slice_num, PWM_CHAN_A, WRAP / 2); // Define o nível do canal A do PWM para 50% do valor de wrap.
    // Estrutura para armazenar o estado do PWM
    pwm_state_t state = {slice_num, true}; // Inicializa a estrutura de estado do PWM.

    // Cria um temporizador que chama pwm_timer_callback após 'duration_ms' milissegundos
    repeating_timer_t timer;              // Declara um temporizador.
    add_repeating_timer_ms(duration_ms, pwm_timer_callback, &state, &timer); // Adiciona um temporizador que chama a função de callback.
}


void setup_gpio(){
    // Inicializa GPIOs
    gpio_init(BOTAO_B);
    gpio_init(BOTAO_A);
    gpio_init(SW);
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    // Define GPIOs como IN ou OUT
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_set_dir(SW, GPIO_IN);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);
    // Define botões como pull-up
    gpio_pull_up(BOTAO_B);
    gpio_pull_up(BOTAO_A); 
    gpio_pull_up(SW);   
}


// função principal
int main()
{
    stdio_init_all();
    init_display();
    setup_gpio();

    // Criação do temporizador
    struct repeating_timer timer;
    add_repeating_timer_ms(100, repeating_timer_callback, NULL, &timer); // Chama a função a cada 100 ms
    uint32_t tempo_estado = 0; // Variável para rastrear o tempo em cada estado
    
    // Declarações da matriz de LED 5X5
    PIO pio = pio0;
    bool ok;
    uint32_t valor_led;
    double r , g , b ;
    uint offset = pio_add_program(pio, &blink_program);
    uint sm = pio_claim_unused_sm(pio, true);
    blink_program_init(pio, sm, offset, MATRIX_PIN);
    uint64_t tempo_anterior = time_us_64(); // Captura o tempo inicial
   // variaveis utilizadas na estrutura de repetição de cada animação da matriz:
    volatile int j = 0;
    volatile int k = 10;
    volatile int m = 21;
    // Iniciando a maquina de estado no estado atual
    Estado estado_atual = ESPERA; 
    while (true) {
        
        // time de 100 ms para quando a maquina de estado reinicia sua etapa para etapa de espera
        if (timer_expired) {
            timer_expired = false; // Reseta a flag
            tempo_estado += 100; // Incrementa o tempo em 100 ms
        }
        
        switch (estado_atual) {
            
            case ESPERA:
            desenho_pio1(matriz_0, valor_led, pio, sm); // Limpa a matriz de Led        
            gpio_put(LED_G, 1);  // INDICA QUE O PROGRAMA ESTA ATIVO 
            ssd1306_draw_string(&ssd, "Sistema ON ", 8, 10); 
            ssd1306_draw_string(&ssd, " ABERTE ", 8, 30); 
            ssd1306_draw_string(&ssd, "O BOTAO B ", 8, 40 );
            ssd1306_send_data(&ssd); // Atualiza o display             
                if (gpio_get(BOTAO_B) == 0) {
                uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time());
                if (tempo_interrup - ultima_interrup > DEBOUNCE) {
                    ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
                    ssd1306_send_data(&ssd); // Atualiza o display 
                    desenho_pio1(matriz_0, valor_led, pio, sm); // Limpa a matriz de Led
                    estado_atual = ENCHER_ING1;
                    tempo_estado = 0; // Reseta o tempo do estado
                }
                }
            break;
            
            case ENCHER_ING1:
            gpio_put(LED_G, 0);   
           
            gpio_put(LED_B, 1); // INDICA QUE O PROGRAMA ESTÁ NA FASE DE RECEBER OS INGREDIENTES           
            while (j < 10) { 
                uint64_t tempo_atual = time_us_64();       
                if (tempo_atual - tempo_anterior >= 1000000) { // Se 1 segundo se passou
                    tempo_anterior = tempo_atual; // Atualiza o tempo de referência
                    desenho_pio1(matrizes[j], valor_led, pio, sm); // Chama a função de desenho
                    j++; // Passa para o próximo quadro da animação               
                }} 
             
                    ssd1306_draw_string(&ssd, "FASE 1", 8, 10); 
                    ssd1306_draw_string(&ssd, "CONCLUIDA", 8, 20); 
                    ssd1306_draw_string(&ssd, " ABERTE ", 8, 30); 
                    ssd1306_draw_string(&ssd, "O JOYSTICK ", 8, 40 );
                    ssd1306_send_data(&ssd); // Atualiza o display
                   
             
        
                if (gpio_get(SW) == 0){
                    uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual
                    if (tempo_interrup - ultima_interrup > DEBOUNCE ) { // Verifica o tempo de debounce               
                           if (tempo_estado >= 3000) { // 3 segundos                            
                            ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
                            ssd1306_send_data(&ssd); // Atualiza o display 
                            estado_atual = ENCHER_ING2;
                            tempo_estado = 0; // Reseta o tempo do estado                           
                        }
                        break;              
                    }
                }
                break;
            
            case ENCHER_ING2:             
              while (k < 20) { 
                uint64_t tempo_atual = time_us_64();
        
                if (tempo_atual - tempo_anterior >= 1000000) { // Se 1 segundo se passou
                    tempo_anterior = tempo_atual; // Atualiza o tempo de referência
                    desenho_pio1(matrizes[k], valor_led, pio, sm); // Chama a função de desenho
                    k++; // Passa para o próximo quadro da animação
                }}
              
                desenho_pio1(matriz_20, valor_led, pio, sm);
                ssd1306_draw_string(&ssd, "FASE 2", 8, 10); 
                ssd1306_draw_string(&ssd, "CONCLUIDA", 8, 20); 
                ssd1306_draw_string(&ssd, " ABERTE ", 8, 30); 
                ssd1306_draw_string(&ssd, "O BOTAO A", 8, 40 );
                ssd1306_send_data(&ssd); // Atualiza o display

            if (gpio_get(BOTAO_A) == 0){
                uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual
                if (tempo_interrup - ultima_interrup > DEBOUNCE ) { // Verifica o tempo de debounce                                
                    if (tempo_estado >= 3000) { // 3 segundos
                        ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
                        ssd1306_send_data(&ssd); // Atualiza o display                        
                        desenho_pio1(matriz_0, valor_led, pio, sm);
                        estado_atual = MISTURAR_AQUECER;
                        tempo_estado = 0; // Reseta o tempo do estado
                    }
                }
                }
                break;
            
            case MISTURAR_AQUECER:  
            ssd1306_draw_string(&ssd, "MISTURANDO", 8, 10); 
            ssd1306_draw_string(&ssd, "E", 8, 20); 
            ssd1306_draw_string(&ssd, "AQUECENDO", 8, 30 );
            ssd1306_draw_string(&ssd, "AGUARDE", 8, 40 );   
            ssd1306_send_data(&ssd); // Atualiza o display              
            desenho_pio1(matriz_f, valor_led, pio, sm);
            gpio_put(LED_B, 0);
            gpio_put(LED_R, 1);
            // Chama a função buzz com o pino do BUZZER e as durações desejadas
            pwm_control(BUZZER,5000);
                                     
            if (tempo_estado >= 5000) { // 10 segundos  
                ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
                ssd1306_send_data(&ssd); // Atualiza o display            
                estado_atual = RESULTADO;
                tempo_estado = 0; // Reseta o tempo do estado
            }
            break;
            
            case RESULTADO:
            desenho_pio1(matriz_21, valor_led, pio, sm);
            ssd1306_draw_string(&ssd, "PROCESSO ", 8, 10); 
            ssd1306_draw_string(&ssd, " FINALIZADO ", 8, 30); 
            ssd1306_send_data(&ssd); // Atualiza o display  
            gpio_put(LED_R, 0);
            
            if (tempo_estado >= 5000) { // 10 segundos  
                ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
                ssd1306_send_data(&ssd); // Atualiza o display            
                estado_atual = DRENAR;
                tempo_estado = 0; // Reseta o tempo do estado
            }
            break;


            case DRENAR:
            
            ssd1306_draw_string(&ssd, "DRENAGEM", 8, 10); 
            ssd1306_draw_string(&ssd, "INICIALIZADA", 8, 30); 
            ssd1306_send_data(&ssd); // Atualiza o display  
            while (m < 29) { 
                uint64_t tempo_atual = time_us_64();
                    if (tempo_atual - tempo_anterior >= 1000000) { // Se 1 segundo se passou
                    tempo_anterior = tempo_atual; // Atualiza o tempo de referência
                    desenho_pio1(matrizes[m], valor_led, pio, sm); // Chama a função de desenho
                    m++; // Passa para o próximo quadro da animação
                }}
            desenho_pio1(matriz_0, valor_led, pio, sm);
            
            if (tempo_estado >= 2000) { // 2 segundos 
                ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
                ssd1306_send_data(&ssd); // Atualiza o display            
                estado_atual = FINALIZAR;
                tempo_estado = 0; // Reseta o tempo do estado
            }
            break;
            
            case FINALIZAR:
            ssd1306_draw_string(&ssd, "FIM", 8, 30); 
            ssd1306_send_data(&ssd); // Atualiza o display 
            if (tempo_estado >= 3000) { // 2  
            ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
            ssd1306_send_data(&ssd); // Atualiza o display 
            estado_atual = ESPERA;
            tempo_estado = 0; // Reseta o tempo do estado
            }
                break;
        }
    }
}



