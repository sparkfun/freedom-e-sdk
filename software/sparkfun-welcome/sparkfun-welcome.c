/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <metal/cpu.h>
#include <metal/gpio.h>


#define RTC_FREQ    32768


// Define our LED PIN. 
// On the SparkFun RED-V the LED PIN is 5
#define RED_V_LED_PIN 5

struct metal_cpu *cpu0;
struct metal_interrupt *cpu_intr, *tmr_intr;
int tmr_id;
volatile uint32_t timer_isr_flag;

void display_banner (void) {

    printf("\n");
    printf("                                      SparkFun Electronics \n");
    printf("\n");
    printf("      ___           ___           ___                    ___                       ___           ___ \n");
    printf("     /\\  \\         /\\  \\         /\\  \\                  /\\  \\          ___        /\\__\\         /\\  \\ \n");
    printf("    /::\\  \\       /::\\  \\       /::\\  \\                /::\\  \\        /\\  \\      /:/  /        /::\\  \\ \n");
    printf("   /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\              /:/\\:\\  \\       \\:\\  \\    /:/  /        /:/\\:\\  \\ \n");
    printf("  /::\\~\\:\\  \\   /::\\~\\:\\  \\   /:/  \\:\\__\\            /::\\~\\:\\  \\      /::\\__\\  /:/__/  ___   /::\\~\\:\\  \\  \n");
    printf(" /:/\\:\\ \\:\\__\\ /:/\\:\\ \\:\\__\\ /:/__/ \\:|__|          /:/\\:\\ \\:\\__\\  __/:/\\/__/  |:|  | /\\__\\ /:/\\:\\ \\:\\__\\  \n");
    printf(" \\/_|::\\/:/  / \\:\\~\\:\\ \\/__/ \\:\\  \\ /:/  /          \\/__\\:\\ \\/__/ /\\/:/  /     |:|  |/:/  / \\:\\~\\:\\ \\/__/ \n");
    printf("    |:|::/  /   \\:\\ \\:\\__\\    \\:\\  /:/  /                \\:\\__\\   \\::/__/      |:|__/:/  /   \\:\\ \\:\\__\\  \n");
    printf("    |:|\\/__/     \\:\\ \\/__/     \\:\\/:/  /                  \\/__/    \\:\\__\\       \\::::/__/     \\:\\ \\/__/   \n");
    printf("    |:|  |        \\:\\__\\        \\::/__/                             \\/__/        ~~~~          \\:\\__\\   \n");
    printf("     \\|__|         \\/__/         ~~                                                             \\/__/  \n");
    printf("\n");

    printf("\n");
    printf("            _______..______      ___      .______       __  ___  _______  __    __  .__   __.      \n");
    printf("           /       ||   _  \\    /   \\     |   _  \\     |  |/  / |   ____||  |  |  | |  \\ |  |  \n");
    printf("          |   (----`|  |_)  |  /  ^  \\    |  |_)  |    |  '  /  |  |__   |  |  |  | |   \\|  |    \n");
    printf("           \\   \\    |   ___/  /  /_\\  \\   |      /     |    <   |   __|  |  |  |  | |  . `  |  \n");
    printf("       .----)   |   |  |     /  _____  \\  |  |\\  \\----.|  .  \\  |  |     |  `--'  | |  |\\   | \n");
    printf("       |_______/    | _|    /__/     \\__\\ | _| `._____||__|\\__\\ |__|      \\______/  |__| \\__| \n");
    printf("\n");
    printf("\n");
    printf("\n");
    printf("                                      Welcome to the future!\n");


}

//////////////////////////////////////////////////////////
void timer_isr (int id, void *data) {

    // Disable Timer interrupt
    metal_interrupt_disable(tmr_intr, tmr_id);

    // Flag showing we hit timer isr
    timer_isr_flag = 1;
}
//////////////////////////////////////////////////////////
// Wait on a timer and set the value of the given pin

void wait_for_timer(struct metal_gpio *gpio0, int thePin, int value) {

    // clear global timer isr flag
    timer_isr_flag = 0;

    // Set timer
    metal_cpu_set_mtimecmp(cpu0, metal_cpu_get_mtime(cpu0) + RTC_FREQ);

    // Enable Timer interrupt
    metal_interrupt_enable(tmr_intr, tmr_id);

    // wait till timer triggers and isr is hit
    while (timer_isr_flag == 0){};

    timer_isr_flag = 0;
    
    metal_gpio_set_pin(gpio0, thePin, value);
}

//////////////////////////////////////////////////////////
int main (void)
{
    int rc;

    // Lets get the CPU and and its interrupt
    cpu0 = metal_cpu_get(0);
    if (cpu0 == NULL) {
        printf("CPU null.\n");
        return 2;
    }
    cpu_intr = metal_cpu_interrupt_controller(cpu0);
    if (cpu_intr == NULL) {
        printf("CPU interrupt controller is null.\n");
        return 3;
    }
    metal_interrupt_init(cpu_intr);

    // display welcome banner
    display_banner();

    // Setup Timer and its interrupt so we can toggle LEDs on 1s cadence
    tmr_intr = metal_cpu_timer_interrupt_controller(cpu0);
    if (tmr_intr == NULL) {
        printf("TIMER interrupt controller is  null.\n");
        return 4;
    }
    metal_interrupt_init(tmr_intr);
    tmr_id = metal_cpu_timer_get_interrupt_id(cpu0);
    rc = metal_interrupt_register_handler(tmr_intr, tmr_id, timer_isr, cpu0);
    if (rc < 0) {
        printf("TIMER interrupt handler registration failed\n");
        return (rc * -1);
    }

    // Lastly CPU interrupt
    if (metal_interrupt_enable(cpu_intr, 0) == -1) {
        printf("CPU interrupt enable failed\n");
        return 6;
    }

    // Setup for toggling our LED Pin - 
    // The Plan
    //   - Get the GPIO devices
    //   - Enable output on the pin, disable input
    //   - turn off the mux for this pin

    //Get gpio device handle
    struct metal_gpio * gpio0 = metal_gpio_get_device(0);
    metal_gpio_enable_output(gpio0, RED_V_LED_PIN);
    metal_gpio_disable_input(gpio0, RED_V_LED_PIN);
   
    // Disable mux on this pin or the LED will not be responsive
    metal_gpio_disable_pinmux(gpio0, RED_V_LED_PIN);
    // Start with the pin LOW
    metal_gpio_set_pin(gpio0, RED_V_LED_PIN, 0);   

    while (1) {

        // Turn on LED 
        wait_for_timer(gpio0, RED_V_LED_PIN, 1);

        wait_for_timer(gpio0, RED_V_LED_PIN, 0);
    }

    // return
    return 0;
}
