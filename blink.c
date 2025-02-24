#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"

//arquivo .pio
#include "blink.pio.h"


#define NUM_PIXELS 25 //número de LEDs
#define OUT_PIN 7 // pino de saída da matriz de led 5x5
#define buzzer 21 // Buzzer para simular o motor acionado por PWM
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


// Função de callback do temporizador
bool repeating_timer_callback(struct repeating_timer *t) {
    timer_expired = true; // Define a flag para indicar que o temporizador expirou
    return true; // Retorna true para continuar chamando a função
}



int PHYSICAL_LEDS_MAPPER[25] = {
    24, 23, 22, 21, 20,
    15, 16, 17, 18, 19,
    14, 13, 12, 11, 10,
    5, 6, 7, 8, 9,
    4, 3, 2, 1, 0};

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g)
{
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (R << 24) | (B << 16) | (G << 8);
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b){

   for (int16_t i = 0; i < NUM_PIXELS; i++)
    {
        int led_matrix_location = PHYSICAL_LEDS_MAPPER[i];
        valor_led = matrix_rgb(r * desenho[led_matrix_location], g * desenho[led_matrix_location], b * desenho[led_matrix_location]);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

double matriz_0[25] = {
    0.0, 0.0, 0.0, 0.0, 0.0,  
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.0
    };
    // Animação do ingrediente 1
    double matriz_1[25] = {
    0.0, 0.0, 0.0, 0.0, 0.0,  
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.1
    };

    double matriz_2[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.1, 0.1
        };

    double matriz_3[25] = {
    0.0, 0.0, 0.0, 0.0, 0.0,  
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.1, 0.1, 0.1
    };
   
    double matriz_4[25] = 
    {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.1, 0.1, 0.1, 0.1
        };

    double matriz_5[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };
    
    double matriz_6[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };
    //7
    double matriz_7[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };
    
    double matriz_8[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };

    double matriz_9[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };

     double matriz_10[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.1, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };



    // Animação para o ingrediente 2 

    double matriz_11[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.1, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };
        double matriz_12[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };
        double matriz_13[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };
        double matriz_14[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.0, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };
        double matriz_15[25] = {
        0.0, 0.0, 0.0, 0.0, 0.0,  
        0.0, 0.0, 0.0, 0.0, 0.0, 
        0.0, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1, 
        0.1, 0.1, 0.1, 0.1, 0.1
        };
    // Ponteiro para armazenar a matriz ativa
    double* matrizes[10] = {
         matriz_1, matriz_2, matriz_3, matriz_4,
        matriz_5, matriz_6, matriz_7, matriz_8, matriz_9, matriz_10
    };


    void loop_com_timer_nao_bloqueante(PIO pio, uint sm, uint r, uint g, uint b, const uint8_t matrizes[][8], uint valor_led) {
        uint64_t tempo_anterior = time_us_64(); // Captura o tempo inicial
        int j = 0;}

int main()
{
    stdio_init_all();
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

    PIO pio = pio0;
    bool ok;
    uint32_t valor_led;
    double r , g , b ;

    r = 1;
    g = 0;
    b = 1;
    uint offset = pio_add_program(pio, &blink_program);
    uint sm = pio_claim_unused_sm(pio, true);
    blink_program_init(pio, sm, offset, OUT_PIN);

    uint64_t tempo_anterior = time_us_64(); // Captura o tempo inicial
    int j = 0;


    Estado estado_atual = ESPERA; 
    while (true) {
       
        if (timer_expired) {
            timer_expired = false; // Reseta a flag
            tempo_estado += 100; // Incrementa o tempo em 100 ms
        }
    
    
        switch (estado_atual) {
            case ESPERA:
            desenho_pio(matriz_0, valor_led, pio, sm, r, g, b);
            gpio_put(LED_G, 1);
            
            if (gpio_get(BOTAO_B) == 0) {
                uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time());
                if (tempo_interrup - ultima_interrup > DEBOUNCE) {
                    estado_atual = ENCHER_ING1;
                    tempo_estado = 0; // Reseta o tempo do estado
                }
            }
            break;
            
            case ENCHER_ING1:
                gpio_put(LED_G, 0);        
                gpio_put(LED_B, 1);

                
                while (j < 10) { 
                    uint64_t tempo_atual = time_us_64();
            
                    if (tempo_atual - tempo_anterior >= 1000000) { // Se 1 segundo se passou
                        tempo_anterior = tempo_atual; // Atualiza o tempo de referência
                        desenho_pio(matrizes[j], valor_led, pio, sm, r, g, b); // Chama a função de desenho
                        j++; // Passa para o próximo quadro da animação
                    }}
            desenho_pio(matriz_10, valor_led, pio, sm, r, g, b);  

                if (gpio_get(SW) == 0){
                    uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual
                    if (tempo_interrup - ultima_interrup > DEBOUNCE ) { // Verifica o tempo de debounce
                
                           if (tempo_estado >= 3000) { // 3 segundos
                            
                            estado_atual = ENCHER_ING2;
                            tempo_estado = 0; // Reseta o tempo do estado

                            
                        }
                        break;
                
                    }
                }
                break;
            
            case ENCHER_ING2:
            gpio_put(LED_B, 0);
            gpio_put(LED_G, 1);
            desenho_pio(matriz_11, valor_led, pio, sm, r, g, b);
           
            if (gpio_get(BOTAO_A) == 0){
                uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual
                if (tempo_interrup - ultima_interrup > DEBOUNCE ) { // Verifica o tempo de debounce
                                   
                    if (tempo_estado >= 3000) { // 3 segundos
                        estado_atual = MISTURAR_AQUECER;
                        tempo_estado = 0; // Reseta o tempo do estado
                    }
                }
                }
                break;
            
            case MISTURAR_AQUECER:
            r = 1;
            b = 0;
            g = 0;
            desenho_pio(matriz_11, valor_led, pio, sm, r, g, b);
            gpio_put(LED_R, 1);
            if (tempo_estado >= 5000) { // 5 segundos
                gpio_put(LED_R, 0);
                estado_atual = DRENAR;
                tempo_estado = 0; // Reseta o tempo do estado
            }
            break;
            
            case DRENAR:
            desenho_pio(matriz_0, valor_led, pio, sm, r, g, b);
            gpio_put(LED_R, 1);
            gpio_put(LED_G, 1);
            gpio_put(LED_B, 1);
            if (tempo_estado >= 10000) { // 10 segundos
                estado_atual = FINALIZAR;
                tempo_estado = 0; // Reseta o tempo do estado
            }
            break;
            
            case FINALIZAR:
            gpio_put(LED_R, 0);
            gpio_put(LED_G, 0);
            gpio_put(LED_B, 0);
                
            estado_atual = ESPERA;
                break;
        }
        sleep_ms(100);
    }
}



