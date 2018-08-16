/*//////////////
 * TCD1304-CCD-Control1
 * controller for CCD
 * re-written 8/8/18 based on ADC development sketch *
 */
/* timer based pwm
   function timer   pin
   f-clock  T0CCP0    PB6
   ICG      WT0CCP0   PC4
   SH       WT1CCP0   PC6
   ADC trigger-T1CCP0    no pin
   ADC      read      PE3
   UART-TX            PA1
   UART-RX            PA0

   synch function cut, pulses synched in ICG interrupt
   data acquisition triggered by IntEnable(INT_ADC0SS3) command
   */

#include <stdint.h>  // from lab 2
#include <stdbool.h>
#include <stdlib.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"  // from lab 3
#include "driverlib/timer.h"       // lab 3
#include "inc/tm4c123gh6pm.h"  //  lab 3
#include "driverlib/debug.h"   // lab 5
#include "driverlib/adc.h"     // lab 5
#include "driverlib/uart.h"    // lab 12
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"     // lab 15
#include "driverlib/rom.h"     // lab 15
#include "inc/hw_gpio.h"       // lab 15
#include "utils/uartstdio.h"   // from single_ended.c
#include "utils/uartstdio.c"
//  from usb_dev_serial.c:
#include "driverlib/fpu.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "utils/ustdlib.h"
#include "usb_serial_structs.h"

// end added includes

// global variables
    volatile uint8_t mode = 0;  // 0: single shot, 1: continuous acq,
    volatile bool runFlag = false;
    volatile bool writeFlag = false;
    volatile uint32_t dataCounter = 0;
    volatile uint32_t integrationTime; // original
    volatile uint8_t loops = 4;
    volatile uint8_t loopCounter = 0;

    volatile uint32_t integrationPeriod; // from ADC dev
 //   pwm variables
    uint32_t ICGPeriod = 1182080;   // 3694 cells, 4 cocks per cell, 80 ticks per clock
    uint8_t clockPeriod = 40;
    uint8_t clockTick = 20;
    uint32_t ICGdelay = 120;               // pulse period for ICG
    uint32_t SHdelay = 56;               // pulse period for SH
           // Integration period in us, convert to clock cycles in initiation loop
    volatile uint32_t integrationPeriod = 1000;
    volatile uint32_t g_ui32SysTickCount = 0;

    // usb crap
    static bool g_bSendingBreak = false;  // break condition flag
    static volatile bool g_bUSBConfigured = false;
    char *g_pcStatus;
    volatile uint32_t g_ui32Flags = 0;
    // Flags used to pass commands from interrupt context to the main loop.
    volatile uint32_t g_ui32TxCount = 0;
    volatile uint32_t g_ui32RxCount = 0;
    #define COMMAND_PACKET_RECEIVED 0x00000001
    #define COMMAND_STATUS_UPDATE   0x00000002


    // data variables
    uint32_t ADCtrigger = 160;  // delay trigger to after second fM clock tick
    volatile uint32_t data[3694] = {0};       // intensity data;
    uint32_t dataPoint[1];
    volatile uint8_t average = 4;     // number of data points to average


    // other random shit that may not be used ever
    volatile int32_t clockCounter = 0;  // want set at -5
    volatile uint32_t clockRoll = 50; // integration time > data points // 3694 points
    bool invert = false;

  // internal function prototypes
    uint32_t readVal();
 //   void synch();
    static void USBEcho();
    void usbRead();
    void usbWrite();
    void initiate();
 //   void initiateRead();
    void writeData();
    void printVal(uint32_t);


    /////////////////////////
    //   Method for inputing integers
    // "TODO" adjust this to use usb input
    /////////////////////////
uint32_t readVal(void){
        bool run = false;
        uint32_t val = 0;
        uint32_t pow[] = {1,10,100,1000,10000,100000};
        uint32_t digit;

        char buffer[64];
        int bytes;
        while(run == false){
        if(UARTCharsAvail(UART0_BASE)){
            bytes = UARTgets(buffer, sizeof(buffer));
           // if ((bytes = UARTgets(buffer, sizeof(buffer)))) {
            UARTprintf("got %d bytes (%s)\n", bytes, buffer);
           // UARTprintf("got %d bytes\n", bytes);
           //             exec_cmd(buffer);
            if(bytes!=0){
                int p;
                for(p=0;p<bytes;p++){
                    digit = (buffer[bytes-1-p]-'0')*pow[p];
                    val += digit;
                }
                UARTprintf("value: %d\n", val);
                        run = true;
            }

        }else{
            // insert delay
        }

        }  // end of run false loop
        return val;
    }
/*void synch(){
    UARTprintf("synched, line 131\n");
    TimerLoadSet(TIMER0_BASE, TIMER_A, clockPeriod -1);  // clock
    TimerLoadSet(TIMER1_BASE, TIMER_A, 4*clockPeriod -1);  // adc
    TimerLoadSet(WTIMER0_BASE, TIMER_A, ICGPeriod -1);
    SysCtlDelay(2);                         // delay between ICG and SH pulses
    TimerLoadSet(WTIMER0_BASE, TIMER_A, integrationPeriod -1);
} */

void initiate(void){   // code replaced with that from adc sketch
    // calculate delay values
    UARTprintf("run initiated, line 141\n");
    integrationPeriod *= 80;  // convert from us to clock cycles
    if(integrationPeriod >= ICGPeriod){
        ICGPeriod = integrationPeriod;
    }else{  // make ICG period an integer multiple of integration period
        int p = ICGPeriod/integrationPeriod;
        ICGPeriod = (p+1)*integrationPeriod;
    }
 //   initiateRead();
//    synch();
}
/*
void shutter(void){  // written for invert is true phasing
    uint8_t delay3 = 20;  // 20 gives standing wave, two pulses
    uint8_t delay4 = 300-delay3;  // 56 with no if statement
    SysCtlDelay(delay3);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
    SysCtlDelay(14);  // SH high
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
    SysCtlDelay(delay4);  // time for SH high
    // UARTEchoSet(false);
}
*/

//  works with uart
//  TODO: make this work with usb
void writeData(void){
    UARTprintf("in writeData loop");
    uint16_t report = 3648/average;    // number of points after averaging
    volatile uint32_t modData[3648];  //={0};  // was 912 move to global?
//    UARTprintf("after modData, line 181\n");
         uint16_t t, n, p;
         /// trim data file

        for(n = 0; n< report; n++){
//         for(n = 0; n< 10; n++){
            modData[n] = 0;
            for(p=0; p<average; p++){
            modData[n] += data[n*average+32+p];
            }
            modData[n] /= average;
         }
 //        UARTprintf("printing 10 values, line 192\n");
         for(t = 0; t< report; t++){  //< 3694; t++){  // modify to remove dummy data
         //    UARTprintf("\n%4d\r",t);
             UARTprintf("%i\n",modData[t]);
         //    UARTprintf("%4d\n",modData[t]);
         }
        UARTprintf("@\n");  // end of data signal
        writeFlag = false;
        runFlag = false;  // end of cycle, exit loop
   }
// handlers for usb
static void
SendBreak(bool bSend)
{
    if(!bSend)  // determine whether to start or stop break
    {
        g_bSendingBreak = false;  // remove break
    }
    else
    {
        g_bSendingBreak = true;  // start break
    }
}  // end of send break
uint32_t
ControlHandler(void *pvCBData, uint32_t ui32Event,
               uint32_t ui32MsgValue, void *pvMsgData)
{
    uint32_t ui32IntsOff;
    switch(ui32Event) // which event
    {
        //
        // We are connected to a host and communication is now possible.
        //
        case USB_EVENT_CONNECTED:         // connection established
            g_bUSBConfigured = true;
            USBBufferFlush(&g_sTxBuffer);
            USBBufferFlush(&g_sRxBuffer);
            ui32IntsOff = ROM_IntMasterDisable();
            g_pcStatus = "Connected";
            g_ui32Flags |= COMMAND_STATUS_UPDATE;
            if(!ui32IntsOff)
            {
                ROM_IntMasterEnable();
            }
            break;
        case USB_EVENT_DISCONNECTED:  // host disconnected
            g_bUSBConfigured = false;
            ui32IntsOff = ROM_IntMasterDisable();
            g_pcStatus = "Disconnected";
            g_ui32Flags |= COMMAND_STATUS_UPDATE;
            if(!ui32IntsOff)
            {
                ROM_IntMasterEnable();
            }
            break;

        case USBD_CDC_EVENT_GET_LINE_CODING:  // return current params
         //   GetLineCoding(pvMsgData);
            break;

        case USBD_CDC_EVENT_SET_LINE_CODING:  // set params
          //  SetLineCoding(pvMsgData);
            break;

        case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
           // SetControlLineState((uint16_t)ui32MsgValue);
            break;

        case USBD_CDC_EVENT_SEND_BREAK:  // send a break
            SendBreak(true);
            break;

       case USBD_CDC_EVENT_CLEAR_BREAK:  // clear break
            SendBreak(false);
            break;

        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
            break;

        default:
#ifdef DEBUG
            while(1);
#else
            break;
#endif

    }

    return(0);
}  // end of control handler

uint32_t
RxHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
          void *pvMsgData)
{
    //
    // Which event are we being sent?
    //
    switch(ui32Event)
    {
      case USB_EVENT_RX_AVAILABLE:  // packet received
        {   USBEcho();
        // new stuff for initiating goes here


            break;
        }

       case USB_EVENT_DATA_REMAINING:
        {    return(0);   }

       case USB_EVENT_REQUEST_BUFFER:
        {  return(0);   }

        default:
#ifdef DEBUG
            while(1);
#else
            break;
#endif
    }
    return(0);
}  // end of RXHandler

uint32_t
TxHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
          void *pvMsgData)
{

    switch(ui32Event)    // Which event have we been sent?
    {
        case USB_EVENT_TX_COMPLETE:
             break;
        default:
#ifdef DEBUG
            while(1);
#else
            break;
#endif

    }
    return(0);
}  // end of tx handler

void readParams(void){
    uint32_t ui32Read; // number of data
//    uint32_t power[] = {100000,10000,1000,100,10,1};
 //   uint8_t string[7];  // array of 7 chars to accept ','
    uint8_t cVal;
    uint8_t comma = ',';
//    uint8_t p;           // keeps track of how many chars we have
    uint8_t count;   // keeps track of how many values we have
    uint32_t value[4];  // number of parameters is 4
    uint32_t number;    // value of one char
//    bool readVal = true;
    bool readCount;

//    uint8_t text[6] = {'i','n',' ','R','D','\n'};
//    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,&text[0], 6);

    for(count = 0; count<4; count++){
        value[count] = 0;
        readCount=true;
        while(readCount == true){
            ui32Read = USBBufferRead((tUSBBuffer *)&g_sRxBuffer, &cVal, 1);
            if(ui32Read){  // if we received data
                if(cVal == comma){
                  readCount = false;    // exit loop
                }else{
                value[count] *= 10;
                number = (cVal-'0');
                value[count] += number;
  /*              uint8_t text[6] = {'R','d','P','a','r','\n'};
                    uint8_t countTxt[4] = {'0','1','2','3'};
                    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,&text[0], 5);
                    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,&countTxt[count], 1);
                    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,&text[5], 1);
 */
                }  // end of else loop
            }  //end of if received data loop

          else{  // if no chars left
           readCount = false;
          }
        }// end of readCount is true loop
        printVal(value[count]);
    }  // end of count loop
//    printVal(12);
} // end of readParams
/*
uint32_t stringToInt(uint8_t string[6]){   // method to convert string to int
    uint32_t value = 0;
    uint32_t power[] = {100000,10000,1000,100,10,1};
    uint32_t digit;
    uint8_t count = 0;    // counter
    bool run = false; // return value when true
    while(run == false){
        for(count = 0;count<6;count++){
            if(string[count]>='0'&& string[count]<='9'){
        digit = string[count]-'0';
        value += digit*power[count];
            }else if(string[count]==','){
                run = true;               // terminate method,
                value /= power[count-1];  // divide by appropriate power
            }else{
                run = true;
                value = 0;
            }
        }
    }
   return(value);
} */
static void
USBEcho()
{
    uint32_t ui32Read;
    uint8_t ui8Char;

    uint8_t ch1[2] = {'&','\n'};
//    uint8_t ch2[2] = {'F', '\n'};
//    uint8_t RT = '\n';
//    uint8_t comma = ',';
//    uint8_t run = '!';

 //   uint32_t testInt = 123456;
 //   uint8_t testC[6];

    // If currently sending a break condition, don't receive more data.
    if(g_bSendingBreak)
    {
        return;
    }

    bool read = true;
//    bool readData;  // new

        while(read == true){  // stay in loop until buffer empty
                // Get a character from the buffer.

        ui32Read = USBBufferRead((tUSBBuffer *)&g_sRxBuffer, &ui8Char, 1);

        // Did we get a character?
        if(ui32Read)
        {
            // Update our count of bytes transmitted via the UART.
                        g_ui32TxCount++;

     /*       if(readData == true){
             string[p] = ui8Char;
             p++;
            }else{  */

           // new stuff
            if(ui8Char == '&'){
  //              p = 0;
    //            count = 0;
  //              readData = true;
                readParams();  // read parameters for experiment from usb
 //               USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,&ch2[0], 2);
//                USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,&RT, 1);
                }
            else if(ui8Char == '*'){
                USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,
                    &ch1[0], 2);
   //             USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,
   //                 &RT, 1);
 /*           } else if(ui8Char == 'T'){  // test for ltoa
              printVal(testInt);  */
            } else {
                // echo character back to com port
                      USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,
                                     (uint8_t *)&ui8Char, 1);
            }
     //  }
        }
        else {   // if no characters left in buffer
            read = false;
     //       readData = false;
            // exit the function.
            return;
    //    }
     }  // end of read data if else if stuff
    }
} // end of usb echo

void printVal(uint32_t value){
//    uint8_t digits[6] = {1,2,3,4,5,6};
  //  uint8_t printNo = 0;
    //uint32_t number = 12;
    uint8_t RT = '\n';
    char testA[1] = {0};
    char testB[2] = {0};
    char testC[3] = {0};
    char testD[4] = {0};
    char testE[5] = {0};
    char testF[6] = {0};

    if(value<10){
    ltoa(value, testA);
    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&testA, 1);
    } else if (value<100){
       ltoa(value, testB);
       USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&testB, 2);
    } else if (value<1000){
        ltoa(value, testC);
        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&testC, 3);
    } else if (value < 10000){
        ltoa(value, testD);
        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&testD, 4);
    } else if (value < 100000){
        ltoa(value, testE);
        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&testE, 5);
    } else {
        ltoa(value, testF);
        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&testF, 6);
    }

 //   ltoa(value, testC); //, 10);  // convert int back to string
    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&testE, 6);
    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&RT, 1);
}

int main(void)
{
    // copied from usb stuff
   // ROM_FPULazyStackingEnable();
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // for usb
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    // Initialize the transmit and receive buffers.
    USBBufferInit(&g_sTxBuffer);
    USBBufferInit(&g_sRxBuffer);
    // Set the USB stack mode to Device mode with VBUS monitoring.
    USBStackModeSet(0, eUSBModeForceDevice, 0);
    // Pass device information to the USB library & place device on bus
    USBDCDCInit(0, &g_sCDCDevice);


    // set up clock, 80 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
 //   SysCtlPWMClockSet(SYSCTL_PWMDIV_8);  // pwm clock at 1 MHz
      // enable peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);  // base A needed for UART

    GPIOPinConfigure(GPIO_PA0_U0RX);   // uart recieve pin
    GPIOPinConfigure(GPIO_PA1_U0TX);   // uart transmit pin
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0|GPIO_PIN_1);  // pins for uart
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000);  // seems to work
    UARTCharPut(UART0_BASE,'*');

    // configure timer1 for ADC trigger
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);   // timer1 for triggering ADC
    SysCtlDelay(3);
    TimerConfigure(TIMER1_BASE, (TIMER_CFG_SPLIT_PAIR|TIMER_CFG_PERIODIC));
    TimerLoadSet(TIMER1_BASE, TIMER_A, 4*clockPeriod -1);  // adc
    //TimerMatchSet(TIMER1_BASE, TIMER_A, ADCtrigger -1);   // comment out? 8/14/18

    TimerUpdateMode(TIMER1_BASE, TIMER_A, (TIMER_UP_LOAD_IMMEDIATE));  //|TIMER_UP_MATCH_IMMEDIATE));
    TimerControlTrigger(TIMER1_BASE,TIMER_A,true);
    TimerADCEventSet(TIMER1_BASE, TIMER_ADC_CAPMATCH_A);  //  trigger ADC
    TimerEnable(TIMER1_BASE, TIMER_A);  // to line 183, timer development

    // configure ADC on PE3, AIN0 //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  //  E3 is AIN0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);   // from lab 5
    SysCtlDelay(3);
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);  // enable pin E3 for ADC reading
    ADCSequenceConfigure(ADC0_BASE,3,ADC_TRIGGER_TIMER,0); // was PWM_MOD1
    ADCSequenceStepConfigure(ADC0_BASE,3,0,ADC_CTL_CH0|ADC_CTL_IE|ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE,3);
    // copied from initiateRead
    ADCIntEnable(ADC0_BASE,3);
    ADCIntClear(ADC0_BASE,3);

    // timer stuff from ADC sketch
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);  // B6 is clock for CCD?
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);  // ICG and SH on C4, 6
    SysCtlDelay(3);
    GPIOPinConfigure(GPIO_PB6_T0CCP0);
    GPIOPinConfigure(GPIO_PC4_WT0CCP0);
    GPIOPinConfigure(GPIO_PC6_WT1CCP0);
    GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinTypeTimer(GPIO_PORTC_BASE, GPIO_PIN_4|GPIO_PIN_6);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);   // timer for clock
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER0);  // wide timer 0 for ICG
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER1);  // wide timer 1 for SH
    SysCtlDelay(3);
    TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_PWM));  // clock timer TIMER_CFG_A_PERIODIC
    TimerConfigure(WTIMER1_BASE, (TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_PWM));   // SH
    TimerConfigure(WTIMER0_BASE, (TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_ONE_SHOT_UP|TIMER_CFG_A_PWM));   // ICG
    TimerControlLevel(WTIMER0_BASE, TIMER_A, true);   // invert output for ICG
    TimerUpdateMode(TIMER0_BASE, TIMER_A, (TIMER_UP_LOAD_IMMEDIATE)); //|TIMER_UP_MATCH_IMMEDIATE));
    TimerUpdateMode(WTIMER0_BASE, TIMER_A, (TIMER_UP_LOAD_IMMEDIATE)); //|TIMER_UP_MATCH_IMMEDIATE));
    TimerUpdateMode(WTIMER1_BASE, TIMER_A, (TIMER_UP_LOAD_IMMEDIATE));  //|TIMER_UP_MATCH_IMMEDIATE));
    TimerLoadSet(TIMER0_BASE, TIMER_A, clockPeriod -1);  // clock
    TimerLoadSet(WTIMER0_BASE, TIMER_A, ICGPeriod -1);   // icg
    TimerLoadSet(WTIMER1_BASE, TIMER_A, integrationPeriod -1);  // sh
    //  define duty cycle for clock, ICG, SH
    TimerMatchSet(TIMER0_BASE, TIMER_A, clockTick -1);
    TimerMatchSet(WTIMER0_BASE, TIMER_A, ICGdelay -1);
    TimerMatchSet(WTIMER1_BASE, TIMER_A, SHdelay -1);

    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(WTIMER1_BASE, TIMER_A);
    TimerEnable(WTIMER0_BASE, TIMER_A);

    IntEnable(INT_WTIMER0A);  // interrupt enable on wide timer 0, A
    TimerIntEnable(WTIMER0_BASE, TIMER_CAPA_EVENT);  // enable interrupts in interrupt controller
        SysCtlDelay(3);


        //////// end of timer stuff

    IntMasterEnable();       // master interrupt enable

    UARTprintf("Master interrupt enabled \n");
 //  synch();
while(1){
      // wait for & signal from com port
  while(runFlag == false){
     if(UARTCharsAvail(UART0_BASE)){
        char c = UARTCharGet(UART0_BASE);
        if(c == '&'){
            c = 0;
            // readVal: uart communication for inputting ints, up to 6 decimals
            int j = readVal();
            integrationTime = j;
             runFlag = true;  // commented out to stay in loop
             initiate();
             IntEnable(INT_ADC0SS3); // initiate read
        }

        else if(c=='*'){                  // GUI seeks connection
            UARTCharPut(UART0_BASE,'&');  // confirm connection to GUI
        }else if (c=='\n'){
        }else{
            UARTprintf("Char %c received\n",c);
        }
      }
    }  // end of while runflag is false loop
  ///////////// initiate data acquisition //////////

if(writeFlag == true){
writeData();
  // print data to uart
}

}  //  end of while(1)
}  // end of main


void
SysTickIntHandler(void)
{
    // Update our system time.
    g_ui32SysTickCount++;  // is this necessary?
}

/*void
USB0DeviceIntHandler(void)  // copied from tivaware
{
    uint32_t ui32Status;

    //
    // Get the controller interrupt status.
    //
    ui32Status = MAP_USBIntStatusControl(USB0_BASE);

    //
    // Call the internal handler.
    //
    USBDeviceIntHandlerInternal(0, ui32Status);
}
*/


void Timer0IntHandler(void)  // lines 44 and 106 in startup_ccs.c file
{
}
void PWM1IntHandler(void){
}

void ADC0IntHandler(void){   // timerDevelopment sketch
    ADCIntClear(ADC0_BASE,3);
    // only record data if writeFlag is false
    if(writeFlag == false){
              ADCSequenceDataGet(ADC0_BASE,3,dataPoint);
              if(loopCounter == 0){
               data[dataCounter] = 0;  // do not record first round data
              } else {
                  data[dataCounter] += dataPoint[0];
              }
               dataCounter++;
     //          UARTprintf("data point: %i \n",dataCounter);
            if(dataCounter == 3694){  // end of read
                UARTprintf("loop: %i finished \n",loopCounter);
                loopCounter++;
                IntDisable(INT_ADC0SS3);
                dataCounter = 0;
          //      ADCSequenceDMADisable(ADC0_BASE,3);
                if(loopCounter == loops){  // if all data has been collected
                    loopCounter = 0;
                    writeFlag = true;  // begin outputting data
                }
            }
    } // end of writeFlag false loop
}

void ICGIntHandler(void){
  //  UARTprintf("ICG interrupt\n");

    //   flag = true;
       TimerIntClear(WTIMER0_BASE,TIMER_CAPA_EVENT);  // event

       TimerLoadSet(TIMER0_BASE, TIMER_A, clockPeriod - 1);  // clock
       TimerLoadSet(WTIMER0_BASE, TIMER_A, ICGPeriod - 1);   // ICG
       SysCtlDelay(2);   // delay between ICG and SH pulses
       TimerLoadSet(WTIMER1_BASE, TIMER_A, integrationPeriod - 1);  // SH
  //     UARTprintf("ICG interrupt, runFlag:%i, writeFlag:%i\n",runFlag,writeFlag);
       if((runFlag == true)&&(writeFlag == false)){  // only during acquisition
           IntEnable(INT_ADC0SS3);
           UARTprintf("Read enabled");
       }
   //    flag = false;
   }
