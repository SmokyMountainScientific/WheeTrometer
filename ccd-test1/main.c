// ccd-test1
// main.c
// Uses 10 MHz pwm to establish 1MHz clock on PD0
// clock counter on pwm initiated interrupt triggers ICG, SH pulses
// ICG and SH operated as GPIO
//   delays in initiate() method keep timing in sync with clock


#include <stdint.h>  // from lab 2
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/tm4c123gh6pm.h"  //  lab 3
#include "driverlib/debug.h"   // lab 5
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"     // lab 15
#include "inc/hw_gpio.h"       // lab 15
#include "driverlib/pwm.h"     // lab 15
/**
 * main.c
 */
    volatile uint32_t clockCounter = 0;
    uint32_t clockRoll = 16000;
    bool invert = false;

void initiate(void){   // initiate the ICG and SH pulses
    uint8_t delay2 = 20;  // 20 gives standing wave, two pulses
    uint8_t delay1 = 56-delay2;  // 56 with no if statement
                                 // 64 gives double standing wave
//if invert is false:
    SysCtlDelay(delay1);  // control phase with clock
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 0);
    SysCtlDelay(2);  // delay between ICG and SH pulses
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 4);
    SysCtlDelay(14);  // time for SH high
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
    SysCtlDelay(14);  // time before ICG goes back to high
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 2);
    SysCtlDelay(delay2);

// if invert is true:
/*    SysCtlDelay(delay1);  // control phase with clock
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 2);
    SysCtlDelay(2);  // delay between ICG and SH pulses
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
    SysCtlDelay(14);  // time for SH high
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
    SysCtlDelay(14);  // time before ICG goes back to high
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
    SysCtlDelay(delay2);
*/

}

int main(void)
{
    uint32_t clockTime = 10;  // gives 1 MHz square wave output  - 1 us period
    uint32_t integrationTime = 4000;  // clock tics per integration -- 4 ms period

    uint32_t delta = 0;   // delta = 10, offset = 8 synchronized pulses with only clock in  up_down mode
    delta += clockTime;
    uint32_t offset = 0;  // 12 K looked pretty close?
      integrationTime *= delta;
      integrationTime += offset;

    uint16_t ICGTime = 5;           // clock tics per ICG pulse
      ICGTime *= clockTime;
       uint16_t SHTime = 2;
      SHTime *= clockTime;

//////////// System clock set to 80 MHz ///////////////////////
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);  // for 80 MHz clock,

///////////// clock out pin //////////////////////////
    SysCtlPWMClockSet(SYSCTL_PWMDIV_8);  // pwm clock at 10 MHz, 100 ns period
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);   // PWM TEST, ICG
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);  // PWM for Clock on PD0
    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);    // pwm pin for clock
    GPIOPinConfigure(GPIO_PD0_M1PWM0);   // clock on D0

    ///////////// ICG and SH pins ////////////////
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);  // PF1 and PF2 for ICG, SH
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2);  // pins for ICG, SH

////////// clock pin ////////////////
    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC); // M0PWM6
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, clockTime);     // clock at 1 MHz
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, clockTime/2);    // PD0

    ////////// ADC trigger ///////////////
    //   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  // PWM for initiating ADC reads
    //   GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4);  // pwm pin for Analog reads
    //   GPIOPinConfigure(GPIO_PE4_M1PWM2);            // ADC trigger on E4
    //   PWMPulseWidthSet(PWM1_BASE, PWM_OUT_2, clocktime*4);     // PE4:  ADC trigger


    ////////////  Enable interrupts on clock ////////////
    IntEnable(INT_PWM1_0);      //  check values
    PWMIntEnable(PWM1_BASE, PWM_INT_GEN_0);   // added 5/30
    IntMasterEnable();
    PWMGenIntTrigEnable(PWM1_BASE,PWM_GEN_0,PWM_INT_CNT_ZERO);

    PWMGenEnable(PWM1_BASE, PWM_GEN_0);

    PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);  //|PWM_OUT_3_BIT, true);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, 2);
    initiate();

 while(1){
  }
}

void PWM1IntHandler(void){
    // clear interrupt
    PWMFaultIntClearExt(PWM1_BASE, PWM_INT_GEN_0);
    clockCounter++;
    if(clockCounter == clockRoll){
        initiate();
        clockCounter = 0;
    }

    PWMFaultIntClearExt(PWM1_BASE, PWM_INT_GEN_0);
}
