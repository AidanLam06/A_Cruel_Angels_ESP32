#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"

#define BUTTON_PIN 21
#define DEBOUNCE_TIME_MS 50
#define PWM_PIN 2
#define REST 0
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_RESOLUTION LEDC_TIMER_8_BIT

//Global variables
TimerHandle_t debounce_timer;
TaskHandle_t buzzer_task_handle = NULL;

const int bpm = 128;
const int quarterNote = 60000 / bpm;

static const char *tag = "MAIN";
volatile int button_pressed = 0;

//enums and arrays for melody
typedef enum {
    WHOLE,
    HALF,
    QUARTER,
    DOTEIGHTH,
    EIGHTH,
    SIXTEENTH
} NoteLength;

typedef enum {
    C4, Db4, D4, Eb4, E4, F4, Gb4, G4, Ab4, A4, Bb4, B4, C5, Db5, D5, Eb5, E5, F5, Gb5, G5, Ab5, A5, Bb5, B5, C6, Db6, D6, Eb6, E6, F6, Gb6, G6, Ab6, A6, Bb6, B6, numNotes 
} Notes;

const float noteFreq[numNotes] = {261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00,
    466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 659.25, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77, 1046.50, 1108.73,
    1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22, 1760.00, 1864.66, 1975.53}; 

const float melody[22] = {
    noteFreq[C5], noteFreq[Eb5], noteFreq[F5], noteFreq[Eb5], noteFreq[F5],
    noteFreq[F5], noteFreq[Bb5], noteFreq[Ab5], noteFreq[G5], noteFreq[F5], noteFreq[G5], REST,
    noteFreq[G5], noteFreq[Bb5], noteFreq[C6], noteFreq[F5], noteFreq[Eb5], 
    noteFreq[Bb5], noteFreq[G5], noteFreq[Bb5], noteFreq[Bb5], noteFreq[C6],
};

const int noteDurations[22] = {
    QUARTER, QUARTER, DOTEIGHTH, DOTEIGHTH, EIGHTH,
    QUARTER, EIGHTH, EIGHTH, SIXTEENTH, EIGHTH, DOTEIGHTH, REST,
    QUARTER, QUARTER, DOTEIGHTH, DOTEIGHTH, EIGHTH,
    QUARTER, EIGHTH, EIGHTH, DOTEIGHTH, QUARTER
};
//end of defintions, enums, arrays and variables

static void IRAM_ATTR button_isr_handler(void* arg) {
    xTimerResetFromISR(debounce_timer, NULL);
}

int getDuration(NoteLength length) {
    switch(length) {
        case WHOLE: return quarterNote * 4;
        case HALF: return quarterNote * 2;
        case QUARTER: return quarterNote;
        case DOTEIGHTH: return quarterNote *3 /4;
        case EIGHTH: return quarterNote / 2;
        case SIXTEENTH: return quarterNote / 4;
        default: return quarterNote;
    }
}

void debounce_timer_callback(TimerHandle_t xTimer) {
    if (gpio_get_level(BUTTON_PIN) == 1) {
        button_pressed = 1;
        ESP_LOGI(tag, "Button Pressed");
    } else {
        button_pressed = 0;
        ESP_LOGI(tag, "Button Released");
    }
}

void ledcWriteTone(ledc_channel_t channel, uint32_t freq) {
    if (freq == 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    } else {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 128);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    }
}

void play_melody() {
    int length = sizeof(melody) / sizeof(melody[0]);
    for (int i = 0; i < length; i++) {
        // if (button_pressed != 1) break;
        ESP_LOGI("MELODY", "playing note %d", i);
        if (melody[i] == REST) {
            int rest = getDuration(EIGHTH);
            ledcWriteTone(LEDC_CHANNEL, 0);
            vTaskDelay(rest / portTICK_PERIOD_MS);
        }
        else {
            int noteDuration = getDuration(noteDurations[i]);
            ledcWriteTone(LEDC_CHANNEL, melody[i]);
            vTaskDelay(noteDuration / portTICK_PERIOD_MS);
            ledcWriteTone(LEDC_CHANNEL, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
    ledcWriteTone(LEDC_CHANNEL, 0);
}

void buzzer_task(void *pvParameter) {
    for (;;) {
        if (button_pressed == 1) {
            ESP_LOGI(tag,"Beginning Melody");
            play_melody();
            if (gpio_get_level(BUTTON_PIN) == 0) button_pressed = 0;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // no busy waiting
    }
}


void app_main(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 2000, // Initial frequency
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PWM_PIN,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ledc_channel);

    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_POSEDGE);

    debounce_timer = xTimerCreate(
    "debounce_timer",
    pdMS_TO_TICKS(DEBOUNCE_TIME_MS),
    pdFALSE,
    NULL,
    debounce_timer_callback
    );

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 5, &buzzer_task_handle);

    ESP_LOGI(tag, "Intialization complete - LEDC, GPIO, ISR and tasks are ready");
}