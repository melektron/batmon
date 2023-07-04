/**
 * @file led.cpp
 * @author melektron
 * @brief status led driver
 * @version 0.1
 * @date 2023-07-04
 *
 * @copyright Copyright FrenchBakery (c) 2023
 *
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <unistd.h>

#include "led.hpp"
#include "env.hpp"
#include "utils.hpp"
#include "log.hpp"

namespace led // private
{
    // led task symbols
    // The statically allocated memory for the task's stack
#define TASK_STACK_SIZE 3000
    StackType_t task_stack[TASK_STACK_SIZE];
    // Handle buffer and handle to led task
    StaticTask_t task_static_buffer;
    TaskHandle_t task_handle = nullptr;
    // Entry point of LED task
    void task_fn(void *);
    
    /**
     * @brief starts the LED task if it is not running already.
     * If it is already running it stops the task and restarts it.
     */
    void start_task();

    /**
     * @brief stops the led task if it is running
     * 
     */
    void stop_task();

    // led state
    enum class led_state_t
    {
        PERMANENT,          // permanent on/off state, no actions need to happen in background
        BLINK_NOTICE_ALIVE, // short pulse every few seconds to indicate the device is turned on
        BLINK_CHARGING,     // 1 Hz 50% flashing indicating battery is being charged
        BLINK_ALARM,        // two quick medium duration pulses every second to indicate an error
    };
    led_state_t led_state = led_state_t::PERMANENT;
};

void led::init()
{
    // turn LED off
    gpio_set_level(env::LED, 0);

    // set to default state
    set_permanent_off();
}

void led::set_permanent_off()
{
    stop_task();
    led_state = led_state_t::PERMANENT;
    gpio_set_level(env::LED, 0);
}
void led::set_permanent_on()
{
    stop_task();
    led_state = led_state_t::PERMANENT;
    gpio_set_level(env::LED, 1);
}
void led::set_blink_notice_alive()
{
    gpio_set_level(env::LED, 0);
    led_state = led_state_t::BLINK_NOTICE_ALIVE;
    start_task();
}
void led::set_blink_charging()
{
    gpio_set_level(env::LED, 0);
    led_state = led_state_t::BLINK_CHARGING;
    start_task();
}
void led::set_blink_alarm()
{
    gpio_set_level(env::LED, 0);
    led_state = led_state_t::BLINK_ALARM;
    start_task();
}

void led::start_task()
{
    if (task_handle != nullptr)
        stop_task();
    
    task_handle = xTaskCreateStatic(
        task_fn,
        "led",
        TASK_STACK_SIZE,
        nullptr,
        1,
        task_stack,
        &task_static_buffer
    );
}
void led::stop_task()
{
    if (task_handle == nullptr)
        return;
    
    vTaskDelete(task_handle);
    task_handle = nullptr;
}

void led::task_fn(void *)
{
    for (;;)
    {
        switch (led_state)
        {
        case led_state_t::PERMANENT:
            sleep(1);
            break;

        case led_state_t::BLINK_NOTICE_ALIVE:
            gpio_set_level(env::LED, 1);
            msleep(100);
            gpio_set_level(env::LED, 0);
            msleep(2900);
            break;

        case led_state_t::BLINK_CHARGING:
            gpio_set_level(env::LED, 1);
            msleep(500);
            gpio_set_level(env::LED, 0);
            msleep(500);
            break;

        case led_state_t::BLINK_ALARM:
            gpio_set_level(env::LED, 1);
            msleep(300);
            gpio_set_level(env::LED, 0);
            msleep(300);
            gpio_set_level(env::LED, 1);
            msleep(300);
            gpio_set_level(env::LED, 0);
            msleep(1100);
            break;

        default:
            LOGE("Invalid led state, turning off");
            set_permanent_off();
            break;
        }
    }
    
    // set to nullptr before deleting, as any code after this line
    // is not run and the variable isn't referenced here anyway
    task_handle == nullptr;
    vTaskDelete(NULL);
}
