/*//////////////
 * WheeTrometer
 * microcontroller firmware for Wheetrometer spectrometer
 *
 *  last copied to GitHub 01/06/20
 *  f clock 1 MHz
 *     'W' char from GUI followed by four chars sets serial number
 *     'A' char from GUI followed by eight chars sets amp, offset
 *     need to remove amp and offset from GUI
 *     need to get chars for amp and offset, convert to uint32_t
 *     ABC values are floats, saved to EEPROM as two uint32_t,
 *     Read from calibration GUI: 'B' followed by 18 chars
 *     sent as chars to spectrometer GUI
 */

/* ////////  amplifier control ////
   function           pin
   amplifier offset   PB7 pwm M0PWM1 (perhaps Timer0, CCP1?)
   gain clock         PA2
   gain chip select   PA3
   gain SDI           PA5

    ///////// pulse signals for CCD //////
   f-clock  T0CCP0    PB6  timer 0 A
   ICG      WT0CCP0   PC4  wide timer 0 A
   SH       WT1CCP0   PC6  wide timer 1 A
   ADC trigger-T1CCP0    no pin
   ADC      read      PE3
   UART-TX            PA1
   UART-RX            PA0

   pulses synched in ICG interrupt


   inputs from GUI:  mode
       integration period
       points to average
       loops

   values called by '#' character sent from GUI via device USB
   */

#include <stdint.h>  // from lab 2
#include <stdbool.h>
#include <stdlib.h>
//#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"                // for SPI
#include "inc/hw_types.h"              // for SPI
#include "driverlib/ssi.h"             // for SPI
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
#include "math.h"
#include "driverlib/eeprom.h"

// added for SPI:
#define NUM_SSI_DATA 8   // not sure if this is used

// serial number & calibration values
    uint32_t ui32Serial;
//    uint32_t ui32Serial;
 uint8_t serial[6] = {'&','0','0','8','0','\n'};  // serial number
 uint8_t ABC0[8] = {'A','2','4','9','.','1','2','\n'};
 uint8_t ABC1[8] = {'B','.','1','4','9','5','2','\n'};
 uint8_t ABC2[8] = {'C','3','2','.','4','7','7','\n'};
 uint32_t ui32ABC0A;  // two words to hold chars in EEPROM
 uint32_t ui32ABC0B;
 uint32_t ui32ABC1A;
 uint32_t ui32ABC1B;
 uint32_t ui32ABC2A;
 uint32_t ui32ABC2B;
 uint8_t offChars[5] = {'=','1','3','5','\n'};
 uint8_t ampChars[5] = {'+','1','7','9','\n'};

//  added for offset
volatile uint32_t offset; // = 135;
volatile uint32_t ampVal; // = 179;   half of 256 range on digital pot is 128

// global variables
   // flags
    volatile uint32_t mode = 0;  // 0: single shot, 1: continuous acq,
    volatile bool runFlag = false;
    volatile bool ADCFlag = false;
    volatile bool startFlag = false;  // Flag set in ICGint, signals time to restart timers
    volatile bool writeFlag = false;
    volatile bool sendFlag = false;  // handshake flag for sending data
    volatile bool inWriteSequence = false;

    static bool g_bSendingBreak = false;  // break condition flag

    // counters & values
    volatile uint32_t dataCounter = 0;
    volatile uint32_t loops = 1;
    volatile uint8_t loopCounter = 0;
    volatile uint8_t count;   // keeps track of how many values we have for parameters
    volatile uint32_t value[6];  // number of parameters is 6, changed from 4
    volatile uint32_t dataCount = 0;  // number of data that have been transferred
    uint8_t comma = ',';

 //   pwm variables
    uint32_t ICGPeriod = 2364160;     // doubled 8/15/19
    // ICGPeriod minimum: 3694 cells, 4 cocks per cell, 0 ticks per clock
    uint32_t ICGdelay = 480;         // width of desired pulse, was 120
    uint32_t ICGMatch;                // pulse period for ICG
    uint8_t clockPeriod = 80;         // clock time doubled 8/15/19, fixed ADC errors
    uint8_t clockTick = 40;

    // Integration period input from GUI in micros, converted to clock cycles during input cycle
    volatile uint32_t integrationPeriod = 80000;  // SH period, 80 K = 1 ms
    uint32_t SHdelay = 240; //56;               // pulse period for SH, 3 us
    uint32_t SHMatch;
    uint32_t ADCLatency = 500;        // delay to deal with latency of ADC

    volatile uint32_t g_ui32SysTickCount = 0;

    // usb crap
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
    volatile uint32_t average = 4;     // number of data points to average
    volatile uint32_t report;         // number of data to report

  // internal function prototypes
 //   void flash();                  // line 112
    void convertData();            // line 117
    void usbRead();
    void usbWrite();
    void writeData();

    void readParams(bool write){
        uint32_t readEEPROM[9];  // serial number: 0
            // ampVal:  1, offset: 2, ABC0:3,4
            // ABC values saved as uint32_t, six digits, first is where dec goes
            //  so 249.12 converts to 324912.
        EEPROMRead(readEEPROM, 0x000, sizeof(readEEPROM));

//////////////// read serial number and convert to char array  //////
        if(readEEPROM[0] != 0){
            ui32Serial = readEEPROM[0];
            serial[1] = (uint8_t)(readEEPROM[0] >> 24);
            serial[2] = (uint8_t)(readEEPROM[0] >> 16);
            serial[3] = (uint8_t)(readEEPROM[0] >> 8);
            serial[4] = (uint8_t)readEEPROM[0];
        }
        if(write== true){
        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &serial[0], 5);  // offset
        }

 //////////  read amp and offset values ////////

        ampVal = readEEPROM[1];
        offset = readEEPROM[2];

   ////  read values for baseline calc's /////////
        if(readEEPROM[3] !=0){
            ABC0[1] = (uint8_t)(readEEPROM[3] >> 24);
            ABC0[2] = (uint8_t)(readEEPROM[3] >> 16);
            ABC0[3] = (uint8_t)(readEEPROM[3] >> 8);
            ABC0[4] = (uint8_t)readEEPROM[3];

            ABC0[5] = (uint8_t)(readEEPROM[4] >> 8);
            ABC0[6] = (uint8_t)(readEEPROM[4]);

            ABC1[1] = (uint8_t)(readEEPROM[5] >> 24);
            ABC1[2] = (uint8_t)(readEEPROM[5] >> 16);
            ABC1[3] = (uint8_t)(readEEPROM[5] >> 8);
            ABC1[4] = (uint8_t)readEEPROM[5];

            ABC1[5] = (uint8_t)(readEEPROM[6] >> 8);
            ABC1[6] = (uint8_t)(readEEPROM[6]);

            ABC2[1] = (uint8_t)(readEEPROM[7] >> 24);
            ABC2[2] = (uint8_t)(readEEPROM[7] >> 16);
            ABC2[3] = (uint8_t)(readEEPROM[7] >> 8);
            ABC2[4] = (uint8_t)readEEPROM[7];

            ABC2[5] = (uint8_t)(readEEPROM[8] >> 8);
            ABC2[6] = (uint8_t)(readEEPROM[8]);

        }
    }

    void writeEEPROM(){
        uint32_t writeVals[9];
        writeVals[0] = ui32Serial;
        writeVals[1] = ampVal;
        writeVals[2] = offset;
        writeVals[3] = ui32ABC0A;
        writeVals[4] = ui32ABC0B;
        writeVals[5] = ui32ABC1A;
        writeVals[6] = ui32ABC1B;
        writeVals[7] = ui32ABC2A;
        writeVals[8] = ui32ABC2B;

        EEPROMProgram(writeVals, 0x000, sizeof(writeVals));
    }

    void setAmp(){       // called line 417
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_1,offset);  // from line 491?
        SSIDataPut(SSI0_BASE,ampVal);  // line 509?
    }

    void convertData(void){   // trim and average data

        uint32_t trim[2] = {0,3648};  // first data point to keep, total number of pixels
    if(average<=0){
        average = 1;
    }
  report = 3648/average;    // number of points after averaging

         uint32_t n, p, conVal;  // conVal is conversion value;

         /// trim data file to remove dummy cells and average data
         for(n =0; n<3648; n++){   // trim off dummy data
             data[n] = data[n+trim[0]];
             data[n] /= (loops-1);
         }
         for (n = trim[1]; n<3694; n++){
                          data[n] = 0;
                      }

         if(average >1){  // dont proceed if average = 1
             for (n=0; n<report; n++){
            conVal = 0;              // problem here when n = 3225
          for(p=0; p<average; p++){
            conVal += data[n*average+p];  // always working with values > n
            }
            data[n] = conVal / average;
         }

         }  // end of if average >1 loop

         inWriteSequence = true;
         writeFlag = true; // allow data write to usb
         writeData();      // new 8/28 ~ 5pm
    } // end of convert data

        void writeData(void){             // runs when called from GUI / usb
            uint8_t end[5] = {'!','!','!','!','\n'};  // changed from @ to !
            uint8_t pause[2] = {'%','\n'};
            uint8_t dot = '.';
            int j;
            int size = 20;

            // convert 20 data to chars
            uint8_t chars[40] = {0};  // keep this 2*size
            uint32_t dat0, dat1;
            for(j = 0;j<size; j++){
                dat0 = data[dataCount+j];
                dat1 = dat0%80;
                dat1 = dat1+48;
                dat0 = dat0/80;
                dat0 = dat0 + 48;
                chars[2*j] = dat0;     // use ascii chars up to zero
                chars[2*j+1] = dat1;
            }

             while(writeFlag == false){}       // stall

           for (j = 0; j<size; j++){
             USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&chars[2*j], 2);
             USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&dot, 1);
             dataCount++;
             if(dataCount == report-1){
                USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&end[0], 5);
                 j = size;
                 dataCount = 0;
                 inWriteSequence = false;
                 IntDisable(INT_WTIMER0A);  // Turn off pulses by disabling ICG interrupts
             }
            }

            writeFlag = false;
           USBBufferWrite((tUSBBuffer *)&g_sTxBuffer,(uint8_t *)&pause[0], 2);

        }
// handlers for usb copied from tivaware cdc serial example
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
    // '&' Initiates read, '*' requires response to GUI
    // 'W': write serial number, writeMem
    // 'A'  amp and offset, writeAmp
    // 'B'  baseline modification

   switch(ui32Event)    // Which event are we being sent?
    {
      case USB_EVENT_RX_AVAILABLE:  // packet received
        {
            bool writeMem = false;  // for writing to EEProm
            bool writeAmp = false;  // for writing amp values to EEProm
            bool writeBase = false; // for writing baseline values
            uint8_t inData[4];
            uint32_t ui32Read;
            uint8_t ui8Char;
            uint8_t got[2] = {'$','\n'};  // acknowledge receipt of params

            // If currently sending a break condition, don't receive more data.
            if(g_bSendingBreak)
            {
                return(0);
            }

   //////  begin reading loop //////////////////
            bool read = true;
                while(read == true){  // stay in loop until buffer empty

                    // Get a character from the buffer.
                ui32Read = USBBufferRead((tUSBBuffer *)&g_sRxBuffer, &ui8Char, 1);

                if(ui32Read)  // if a char was received
                {

      ////////// looking for different control signals ///////////////
                    if(writeMem == true){    // read values for serial number
                                             // can use any char

                        inData[count]= ui8Char;
                        count++;
                        if(count == 4){
                            writeMem = false;
                           ui32Serial = (uint32_t)inData[3] | (uint32_t)(inData[2] << 8)
                                   | (uint32_t)(inData[1] << 16) | (uint32_t)(inData[0] << 24);
                          serial[1] = inData[0];
                          serial[2] = inData[1];
                          serial[3] = inData[2];
                          serial[4] = inData[3];

                          USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &serial[0], 6);
                          writeEEPROM();
                        }
                    } else if (writeBase == true){
                        count++;
                        if(count < 5){
                          ui32ABC0A = (ui32ABC0A << 8)+ (uint32_t)ui8Char;
                        }else if (count <7){
                          ui32ABC0B = (ui32ABC0B << 8)+ (uint32_t)ui8Char;
                        }else if (count < 11){
                          ui32ABC1A = (ui32ABC1A << 8)+ (uint32_t)ui8Char;
                        }else if (count < 13){
                          ui32ABC1B = (ui32ABC1B << 8)+ (uint32_t)ui8Char;
                        }else if (count < 17){
                          ui32ABC2A = (ui32ABC2A << 8)+ (uint32_t)ui8Char;
                        }else {
                          ui32ABC2B = (ui32ABC2B << 8)+ (uint32_t)ui8Char;
                        }
                       writeEEPROM();
                       readParams(false);
                    }  // end of writeBase is true loop
                    else if(writeAmp == true){
                        count++;
                        if(count < 4){
                        ampChars[count]=ui8Char;
                        ampVal *= 10;
                        ampVal = ampVal + (uint32_t)(ui8Char-'0');
                        } else {

                        offChars[count-3] =ui8Char;
                        offset *= 10;
                        offset = offset + (uint32_t)(ui8Char-'0');
                        }
        //                if(count == 3){     }
                        if(count == 6){
                            writeAmp = false;
                            writeEEPROM();
                            setAmp();
                            USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &ampChars[0], 5);  // ampVal
                            USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &offChars[0], 5);  // offset
                            }
                    }

                    else if(ui8Char == '*'){          // respond to GUI
                    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &serial[0], 6); // changed size
                    } else if(ui8Char == '@'){

                       ampChars[1] = (uint8_t)(ampVal/100)+'0';
                        ampChars[2] = (uint8_t)(ampVal%100);
                        ampChars[3] = (uint8_t)(ampChars[2]%10)+'0';
                        ampChars[2] = ampChars[2]/10+'0';

                        offChars[1] = (uint8_t)(offset/100)+'0';
                        offChars[2] = (uint8_t)(offset%100);
                        offChars[3] = (uint8_t)(offChars[2]%10)+'0';
                        offChars[2] = offChars[2]/10+'0';

                        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &ABC0[0], 8);  // send calibration vals
                        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &ABC1[0], 8);
                        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &ABC2[0], 8);
                        // get amp and offset values from EEPROM

                        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &ampChars[0], 5);  // ampVal
                        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &offChars[0], 5);  // offset
                   //// end of respond to @ char input

              } else if(ui8Char=='W'){  // write serial number to EEProm
                  writeMem = true;
                  count = 0;
              } else if(ui8Char=='A'){  // write amp gain and offset to EEProm
                  ampVal = 0;
                  offset = 0;
                  writeAmp = true;
                  count = 0;
              } else if(ui8Char=='B'){  // write wavelength calibration data to EEProm
                  writeBase = true;
                  count = 0;
                } else if(ui8Char == '&'){  // start of data read
                        sendFlag = true;  // reads values when true
                        ui8Char = 0;  // prevent repeating this.
                        count = 0;
                        value[0] = 0;

                } else if(ui8Char == '#' && inWriteSequence == true){  // trigger data send to GUI
                    ui8Char = 0;  // prevent repeating this.
                    writeFlag  = true;
                    writeData();

                } else if(sendFlag == true && ui8Char >= '0' && ui8Char <='9'){
            ////  collect input for values///////////
                              value[count] *=10;
                              value[count] += (ui8Char - '0');

                } else if (sendFlag == true && ui8Char == ','){  // end of chars for value input
                        if(count == 3) {  // end of value, changed from 5
                             sendFlag = false;  // stop accepting input
                             ui8Char = ' ';     // '\n';

           /////////  assign values  to parameters //////////
                             mode = value[0];
                             integrationPeriod = value[1]*80; // convert from us to clock cycles
                             average = value[2];  // number of data points to average
                             loops = value[3]+1;    // number of acquisition loops

          ///////// evaluation loop /////////////////////

                                  // calculate ICG period
                            if(integrationPeriod >= ICGPeriod){
                                  ICGPeriod = integrationPeriod;  // - ADCLatency;
                             }else{
                                 // make ICG period an integer multiple of integration period
                                  uint32_t p = 1182080/integrationPeriod;
                                  ICGPeriod = (p+1)*integrationPeriod;  // - ADCLatency;
                             }
                             SysCtlDelay(8000000);  // 100 ms delay?
                             USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, &got[0], 2);  // acknowledge receipt of params

                             dataCount = 0;

                             IntEnable(INT_WTIMER0A);  // ICG

                             TimerIntClear(WTIMER0_BASE,TIMER_CAPA_EVENT);  // event

                                TimerLoadSet(TIMER0_BASE, TIMER_A, clockPeriod - 1);  // clock
                                TimerLoadSet(WTIMER0_BASE, TIMER_A, ICGPeriod - 1);   //1182079);  // ICG
                                TimerMatchSet(WTIMER0_BASE, TIMER_A, ICGPeriod - ICGdelay-1);  //1182079-360);  // ICG

                                TimerLoadSet(WTIMER1_BASE, TIMER_A, integrationPeriod - 1);  // SH
                                SHMatch = integrationPeriod - SHdelay -1;
                                ICGMatch = ICGPeriod - ICGdelay -1;
                                TimerMatchSet(WTIMER1_BASE, TIMER_A, SHMatch);
                                TimerMatchSet(WTIMER0_BASE, TIMER_A, ICGMatch);

                             runFlag = true;
                             sendFlag = false;
                        } // end of if count is 3
                        else {
                            count++;
                            value[count] = 0;
                        }
                    } else {} // end of comma char loop
                }  // end of if there is a char to read
                else {   // if no characters left in buffer
                    read = false;
                    return(0);
             }  // end of read data if / else stuff
            }  // end of while read is true loop
            break;
        }  // end of case where char available

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


int main(void)
{

    uint32_t ui32EEPROMInit;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    //
    // Wait for the EEPROM module to be ready.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0))
    {
    }
    //
    // Wait for the EEPROM Initialization to complete
    //
    ui32EEPROMInit = EEPROMInit();
    //
    // Check if the EEPROM Initialization returned an error
    // and inform the application
    //
    if(ui32EEPROMInit != EEPROM_INIT_OK)
    {
    while(1)
    {
    }
    }


    uint32_t p = ICGPeriod/ integrationPeriod;
    ICGPeriod -= ADCLatency;
     ICGMatch = (p+1)*integrationPeriod - ICGdelay - 1; //ADCLatency - 1;

    uint32_t offsetPeriod;
    offsetPeriod = 256;  //256;
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

    // set up clock
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
      // enable peripherals
    readParams(false);

    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);  // pwm clock
       SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);  // B6 is clock for CCD, B7 for amp offset
       SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
       GPIOPinTypePWM(GPIO_PORTB_BASE,GPIO_PIN_7);
       GPIOPinConfigure(GPIO_PB7_M0PWM1);
       PWMGenConfigure(PWM0_BASE,PWM_GEN_0,PWM_GEN_MODE_DOWN);
       PWMGenPeriodSet(PWM0_BASE,PWM_GEN_0,offsetPeriod);
       PWMPulseWidthSet(PWM0_BASE,PWM_OUT_1,offset);
       PWMOutputState(PWM0_BASE,PWM_OUT_1_BIT,true);
       PWMGenEnable(PWM0_BASE,PWM_GEN_0);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);    // enable SPI module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);  // base A needed for UART (also SPI)

    GPIOPinConfigure(GPIO_PA2_SSI0CLK);  // SPI clock
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);  // SPI chip select
    GPIOPinConfigure(GPIO_PA5_SSI0TX);   // SPI MOSI
    GPIOPinTypeSSI(GPIO_PORTA_BASE,GPIO_PIN_5|GPIO_PIN_3|GPIO_PIN_2);  // SPI pin type

    SSIConfigSetExpClk(SSI0_BASE,SysCtlClockGet(),SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER,10000,8);
    SSIEnable(SSI0_BASE);
//    uint32_t ampVal = ;  // moved up
    SSIDataPut(SSI0_BASE,ampVal);

    // configure timer1 for ADC trigger
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);   // timer1 for triggering ADC
    SysCtlDelay(3);
    TimerConfigure(TIMER1_BASE, (TIMER_CFG_SPLIT_PAIR|TIMER_CFG_PERIODIC));
    TimerLoadSet(TIMER1_BASE, TIMER_A, 4*clockPeriod -1);  // adc

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
    IntEnable(INT_ADC0SS3);  // added 01/17/2019

    // timer stuff from ADC sketch
        // enable gpio b moved up to pwm stuff, line 492 ish
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
    TimerConfigure(WTIMER0_BASE, (TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_ONE_SHOT|TIMER_CFG_A_PWM));
    TimerControlLevel(WTIMER0_BASE, TIMER_A, true);   // invert output for ICG-flipped Jan 13, 2019
    TimerControlLevel(WTIMER1_BASE, TIMER_A, false);   // invert output for SH
    TimerControlLevel(TIMER0_BASE, TIMER_A, true);   // invert clock
    TimerUpdateMode(WTIMER1_BASE, TIMER_A, (TIMER_UP_LOAD_IMMEDIATE));
    TimerUpdateMode(WTIMER0_BASE, TIMER_A, (TIMER_UP_LOAD_IMMEDIATE|TIMER_UP_MATCH_IMMEDIATE));
    TimerUpdateMode(WTIMER1_BASE, TIMER_A, (TIMER_UP_LOAD_IMMEDIATE|TIMER_UP_MATCH_IMMEDIATE));
    SHMatch = integrationPeriod - SHdelay -1;

    TimerLoadSet(TIMER0_BASE, TIMER_A, clockPeriod -1);  // clock
    TimerLoadSet(WTIMER0_BASE, TIMER_A, ICGPeriod -1);  // 1182079);  //ICGPeriod-1);   // icg
    TimerLoadSet(WTIMER1_BASE, TIMER_A, integrationPeriod-1);  // sh
    //  define duty cycle for clock, ICG, SH
    TimerMatchSet(TIMER0_BASE, TIMER_A, clockTick -1);
    TimerMatchSet(WTIMER0_BASE, TIMER_A, ICGMatch);
    TimerMatchSet(WTIMER1_BASE, TIMER_A, SHMatch);

    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(WTIMER1_BASE, TIMER_A);
    TimerEnable(WTIMER0_BASE, TIMER_A);

    TimerIntEnable(WTIMER0_BASE, TIMER_CAPA_EVENT);  // enable interrupts in interrupt controller
        SysCtlDelay(3);

        //////// end of timer stuff

    IntMasterEnable();       // master interrupt enable

while(1){
}
}  // end of main


void
SysTickIntHandler(void)
{
    // Update our system time.
    g_ui32SysTickCount++;  // is this necessary?
}


void Timer0IntHandler(void)  // lines 44 and 106 in startup_ccs.c file
{
}
void PWM1IntHandler(void){
}

void ADC0IntHandler(void){   // timerDevelopment sketch

    uint32_t dataPoint[1]; // moved to make a variable local
    ADCIntClear(ADC0_BASE,3);

    if(ADCFlag == true){
        while(!ADCIntStatus(ADC0_BASE,3,false)){}
           ADCSequenceDataGet(ADC0_BASE,3,dataPoint);

            if(loopCounter == 0){
               data[dataCounter] = 0;  // do not record first round data
              } else {
                  data[dataCounter] += dataPoint[0]; // was dataPoint[0]
              }
               dataCounter++;

              if(dataCounter == 3694){  // end of read
                loopCounter++;
                ADCFlag = false;
                dataCounter = 0;
                if(loopCounter == loops){  // if all data has been collected
                    loopCounter = 0;
                    runFlag = false;  // end run
                    convertData();
                    // begin outputting data
                }
            }
    } // end of ADCFlag is true loop
}

void ICGIntHandler(void){

    TimerIntClear(WTIMER0_BASE,TIMER_CAPA_EVENT);  // event
    if(runFlag == true){

       TimerLoadSet(TIMER0_BASE, TIMER_A, clockPeriod - 1);  // clock
       TimerLoadSet(WTIMER0_BASE, TIMER_A, ICGPeriod - 1);   //1182079);  // ICG

       TimerMatchSet(WTIMER0_BASE, TIMER_A, ICGPeriod - ICGdelay-1);
       TimerLoadSet(WTIMER1_BASE, TIMER_A, integrationPeriod - 1);  // SH

       TimerLoadSet(TIMER1_BASE, TIMER_A, 4*clockPeriod - 1);  // ADC period, added 01/17/2019

       ADCFlag = true;  // move interrupt enable ADC to while 1 loop
       startFlag = true;
    }
   }


