#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"


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



    Estado estado_atual = ESPERA; 
    while (true) {
       
        if (timer_expired) {
            timer_expired = false; // Reseta a flag
            tempo_estado += 100; // Incrementa o tempo em 100 ms
        }
    
    
        switch (estado_atual) {
            case ESPERA:
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
            gpio_put(LED_R, 1);
            if (tempo_estado >= 5000) { // 5 segundos
                gpio_put(LED_R, 0);
                estado_atual = DRENAR;
                tempo_estado = 0; // Reseta o tempo do estado
            }
            break;
            
            case DRENAR:
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
