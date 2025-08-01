#include <stdio.h>
#include "driver/gpio.h"
#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#define LED_PIN GPIO_NUM_2
#define BUTTON_PIN GPIO_NUM_0 

static TimerHandle_t blink_timer;
static TimerHandle_t button_timer;
static QueueHandle_t button_queue;
static volatile int press_time = 0;

void IRAM_ATTR button_isr_handler(void *arg) {
    static int64_t start_time = 0;
    int level = gpio_get_level(BUTTON_PIN);
    if (level == 1) {  
        press_time = (esp_timer_get_time() - start_time) 
        xQueueSendFromISR(button_queue, &press_time, NULL);  
    } else { 
        start_time = esp_timer_get_time();  
        gpio_set_level(LED_PIN, 1); 
    }
}

void blink_callback(TimerHandle_t xTimer) {
    static int state = 0;
    gpio_set_level(LED_PIN, state);
    state = !state;  
}

void button_task(void *arg) {
    int duration;
    while (1) {
        if (xQueueReceive(button_queue, &duration, portMAX_DELAY)) {
            gpio_set_level(LED_PIN, 0);  
            xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(duration), 0);  
            xTimerStart(blink_timer, 0);  
            vTaskDelay(pdMS_TO_TICKS(duration * 2));  
            xTimerStop(blink_timer, 0); 
        }
    }
}

void app_main() {
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0); 

    gpio_pad_select_gpio(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);  

    button_queue = xQueueCreate(10, sizeof(int));  
    blink_timer = xTimerCreate("Blink Timer", pdMS_TO_TICKS(100), pdTRUE, NULL, blink_callback);  

    xTaskCreate(button_task, "Button Task", 2048, NULL, 10, NULL); 
    
    //fin app_main
    
}