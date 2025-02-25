#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
//arquivo .pio
#include "blink.pio.h"

#define CLOCK_DIV 64.0 
#define TPWM 20000.0    
#define WRAP 39063

#define I2C_PORT i2c1
#define I2C_SDA 14 // GPIO SDA DO DISPLAY
#define I2C_SCL 15 // GPIO SCL DO DISPLAY
#define endereco 0x3C // ENDEREÇO DO DISPLAY

#define NUM_PIXELS 25 //número de LEDs
#define OUT_PIN 7 // pino de saída da matriz de led 5x5
#define buzzer 10 // Buzzer para simular o motor acionado por PWM
#define LED_R 13 // GPIO LED VERMELHO
#define LED_B 12 // GPIO LED AZUL
#define LED_G 11 // GPIO LED VERDE
#define BOTAO_A 05
#define BOTAO_B 06 // Define o botão b com Start
#define SW 22
#define DEBOUNCE 200
volatile uint32_t ultima_interrup = 0; // Para armazenar o último tempo de interrupção
volatile bool timer_expired = false; // Flag para indicar se o temporizador expirou

// Definição dos estados da máquina de estados
typedef enum {
    ESPERA,
    ENCHER_ING1,
    ENCHER_ING2,
    MISTURAR_AQUECER,
    DRENAR,
    FINALIZAR
} Estado;
Estado estado_atual = ESPERA;



ssd1306_t ssd; // Inicializa a estrutura do display
void init_display(){
    i2c_init(I2C_PORT, 400 * 1000); // Inicialização do I2C utilizando 400Khz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define GPIO como I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Define GPIO como I2C
    gpio_pull_up(I2C_SDA); // Define GPIO SDA como pullup
    gpio_pull_up(I2C_SCL); // Define GPIO SCL como pullup
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_fill(&ssd, false); // Limpa o display
}


// Função de callback do temporizador
bool repeating_timer_callback(struct repeating_timer *t) {
    timer_expired = true; // Define a flag para indicar que o temporizador expirou
    return true; // Retorna true para continuar chamando a função
}

int MAPPER[25] = {
    24, 23, 22, 21, 20,
    15, 16, 17, 18, 19,
    14, 13, 12, 11, 10,
    5, 6, 7, 8, 9,
    4, 3, 2, 1, 0};

typedef struct {
   volatile uint8_t r; // Red
    volatile uint8_t g; // Green
   volatile uint8_t b; // Blue

} Cor;
uint32_t matrix_rgb1(Cor cor) {
    return (cor.r << 24) | (cor.b << 16) | (cor.g << 8);
}
// int16_t i = NUM_PIXELS - 1; i >= 0; i--
void desenho_pio1(Cor *desenho, uint32_t valor_led, PIO pio, uint sm) {
    for (int16_t i = 0; i < NUM_PIXELS; i++) { // Acessa de trás para frente
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

    // Ponteiro para armazenar a matriz ativa
    Cor* matrizes[20] = {
    matriz_1, matriz_2, matriz_3, matriz_4, matriz_5, matriz_6, matriz_7, matriz_8, matriz_9, matriz_10, matriz_11,
    matriz_12, matriz_13, matriz_14, matriz_15, matriz_16, matriz_17, matriz_18, matriz_19, matriz_20 
    };


    void loop_com_timer_nao_bloqueante(PIO pio,  const uint8_t matrizes[][8], uint valor_led) {
        uint64_t tempo_anterior = time_us_64(); // Captura o tempo inicial
        int j = 0;}
    
    void loop_com_timer(PIO pio,  const uint8_t matrizes[][8], uint valor_led) {
            uint64_t tempo_anterior = time_us_64(); // Captura o tempo inicial
            int k = 10;}

// PWM do buzzer para simular um motor 
void setup_pwm(uint slice_num, uint channel) {
    // Configura o divisor de clock
    pwm_set_clkdiv(slice_num, CLOCK_DIV); // Define o divisor de clock
    pwm_set_wrap(slice_num, WRAP); // Define o valor de wrap
    pwm_set_enabled(slice_num, true); // Habilita o PWM
}
void buzz(uint buzzer_pin, int low_duration, int high_duration) {
    gpio_set_function(buzzer_pin, GPIO_FUNC_PWM); // Configura o pino como PWM

    uint slice_num = pwm_gpio_to_slice_num(buzzer_pin);
    uint channel = pwm_gpio_to_channel(buzzer_pin);

    setup_pwm(slice_num, channel); // Configura o PWM

    absolute_time_t start_time;
    volatile int state = 0; // Estado do buzzer
    volatile int duration = 0; // Duração do estado atual

    while (state <= 7) {  // Agora o loop garante que será finalizado
        switch (state) {
            case 0: // Período baixo
                pwm_set_chan_level(slice_num, channel, 19532); // 25% de duty cycle
                duration = low_duration; 
                start_time = get_absolute_time(); 
                state = 1;
                break;

            case 1: // Espera pelo tempo
                if (absolute_time_diff_us(start_time, get_absolute_time()) >= duration * 1000) {
                    state = 2;
                }
                break;

            case 2: // Período alto
                pwm_set_chan_level(slice_num, channel, 35157); // 50% de duty cycle
                duration = high_duration;
                start_time = get_absolute_time(); 
                state = 3;
                break;

            case 3: // Espera pelo tempo
                if (absolute_time_diff_us(start_time, get_absolute_time()) >= duration * 1000) {
                    state = 4;
                }
                break;

            case 4: // Período baixo novamente
                pwm_set_chan_level(slice_num, channel, 19532); // 25% de duty cycle
                duration = low_duration;
                start_time = get_absolute_time();
                state = 5;
                break;

            case 5: // Espera pelo tempo
                if (absolute_time_diff_us(start_time, get_absolute_time()) >= duration * 1000) {
                    state = 6;
                }
                break;

            case 6: // Desligar o buzzer
                pwm_set_chan_level(slice_num, channel, 0); // 0% de duty cycle
                duration = low_duration;
                start_time = get_absolute_time();
                state = 7;
                break;

            case 7: // Espera pelo tempo final e sai
                if (absolute_time_diff_us(start_time, get_absolute_time()) >= duration * 1000) {
                    state = 8; // Estado final para sair do loop
                }
                break;
        }
    }
}



// função principal
int main()
{
    stdio_init_all();
    init_display();
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);

    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);


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
    blink_program_init(pio, sm, offset, OUT_PIN);

    uint64_t tempo_anterior = time_us_64(); // Captura o tempo inicial
    volatile int j = 0;
    volatile int k = 10;
    // Colocando a maquina no estado inicial que é a espera do botão de start definido como Botâo B
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
            gpio_put(LED_G, 0);  // LEMBRAR DE ALTERAAAAAAAAAAAAAAAAAAAAA PARA 1
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
            gpio_put(LED_B, 0);   // LEMBRAR DE ALTERAAAAAAAAAAAAAAAAAAAAA PARA 1            
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
           
            // Chama a função buzz com o pino do buzzer e as durações desejadas
            // buzz(LED_R, 2000, 4000);
           
            
                                     
            if (tempo_estado >= 5000) { // 10 segundos  
                ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
                ssd1306_send_data(&ssd); // Atualiza o display            
                estado_atual = DRENAR;
                tempo_estado = 0; // Reseta o tempo do estado
            }
            break;
            
            case DRENAR:
            desenho_pio1(matriz_21, valor_led, pio, sm);
            ssd1306_draw_string(&ssd, "PROCESSO ", 8, 10); 
            ssd1306_draw_string(&ssd, " FINALIZADO ", 8, 30); 
            ssd1306_send_data(&ssd); // Atualiza o display  
            if (tempo_estado >= 8000) { // 10 segundos 
                gpio_put(LED_R, 0);// LEMBRAR DE ALTERAAAAAAAAAAAAAAAAAAAAA PARA 1
                gpio_put(LED_G, 0);// LEMBRAR DE ALTERAAAAAAAAAAAAAAAAAAAAA PARA 1
                gpio_put(LED_B, 0);  // LEMBRAR DE ALTERAAAAAAAAAAAAAAAAAAAAA PARA 1
                ssd1306_fill(&ssd, false); // false para limpar (preencher com preto)
                ssd1306_send_data(&ssd); // Atualiza o display            
                estado_atual = FINALIZAR;
                tempo_estado = 0; // Reseta o tempo do estado
            }
            break;
            
            case FINALIZAR:
            desenho_pio1(matriz_0, valor_led, pio, sm);
            if (tempo_estado >= 3000) { // 2  
            gpio_put(LED_R, 0);
            gpio_put(LED_G, 0);
            gpio_put(LED_B, 0);  
            estado_atual = ESPERA;
            tempo_estado = 0; // Reseta o tempo do estado
            }
                break;
        }
        sleep_ms(100);
    }
}



