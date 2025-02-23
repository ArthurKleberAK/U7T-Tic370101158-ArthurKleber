#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"


#define buzzer 21 // Buzzer para simular o motor acionado por PWM
#define LED_R 13 // GPIO LED VERMELHO
#define LED_B 12 // GPIO LED AZUL
#define LED_G 11 // GPIO LED VERDE
#define BOTAO_A 05
#define BOTAO_B 06 // Define o botão b com Start
#define SW 22
#define DEBOUNCE 200
volatile uint32_t ultima_interrup = 0; // Para armazenar o último tempo de interrupção


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

    Estado estado_atual = ESPERA; 
    while (true) {
        
    
    
        switch (estado_atual) {
            case ESPERA:
            gpio_put(LED_G, 1);
            
            if (gpio_get(BOTAO_B)==0){
                uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual
                if (tempo_interrup - ultima_interrup > DEBOUNCE ) { // Verifica o tempo de debounce
                    
                    estado_atual = ENCHER_ING1;
                }
            }
                break;
            
            case ENCHER_ING1:
                gpio_put(LED_G, 0);
                sleep_ms(2000);
                gpio_put(LED_B, 1);
               
                if (gpio_get(SW) == 0){
                    uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual
                    if (tempo_interrup - ultima_interrup > DEBOUNCE ) { // Verifica o tempo de debounce
                       
                        
                     
                        estado_atual = ENCHER_ING2;
                    }
                }
                break;
            
            case ENCHER_ING2:
            gpio_put(LED_B, 0);
            gpio_put(LED_G, 1);
            
            sleep_ms(3000);
           
            gpio_put(LED_G, 0);
            if (gpio_get(BOTAO_A) == 0){
                uint32_t tempo_interrup = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual
                if (tempo_interrup - ultima_interrup > DEBOUNCE ) { // Verifica o tempo de debounce
                                   
                    estado_atual = MISTURAR_AQUECER;
                }
                }
                break;
            
            case MISTURAR_AQUECER:
                gpio_put(LED_R, 1);
                sleep_ms(5000);
                gpio_put(LED_R, 0);           
                estado_atual = DRENAR;
                break;
            
            case DRENAR:
                gpio_put(LED_R, 1);
                gpio_put(LED_G, 1);
                gpio_put(LED_B, 1);
                sleep_ms(3000);
                estado_atual = FINALIZAR;

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
