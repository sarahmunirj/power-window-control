
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.c"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.c"
#include "driverlib/gpio.h"

/*
  These defines help if you want to change the LED pin or the Button pin.
  Remember if you change to a diferent GPIO you need to enable the system
  clock on it
*/
uint32_t SystemCoreClock = 16000000;
#define LED_PERIPH SYSCTL_PERIPH_GPIOF
#define LED_BASE GPIO_PORTF_BASE
#define RED_LED GPIO_PIN_1

#define Button_PERIPH SYSCTL_PERIPH_GPIOC
#define ButtonBase GPIO_PORTC_BASE
#define Button GPIO_PIN_7
#define ButtonInt GPIO_INT_PIN_7

volatile  uint8_t state=0;

void PortFIntHandler(){
  uint32_t status=0;
	SysCtlDelay(50000);
  status = GPIOIntStatus(ButtonBase,true);
  GPIOIntClear(ButtonBase,status);

  if((status & ButtonInt) == ButtonInt){
    //Then there was a Button pin interrupt
    uint8_t value=0;
    
    value= GPIOPinRead(ButtonBase,Button);
    
    if( value==0)
      state^=RED_LED;

    GPIOPinWrite(LED_BASE,RED_LED, state);
    
    /*
      This delay is for deboucing but since it's in a interrupt it
      should be used a better method that is faster
    */    
  }  
} 

int main(void)
{
 
 //Set the clock to 80Mhz
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);

  /*
    No need to enable the button peripheral since it's the same as the LED
    in this case
  */
  SysCtlPeripheralEnable(LED_PERIPH);
  SysCtlDelay(3);
  SysCtlPeripheralEnable(Button_PERIPH);
  SysCtlDelay(3);
  /*
    Configure the switch on the left of the launchpad, GPIO_PIN_4 to a input with
    internal pull-up.
  */
	GPIOPinTypeGPIOOutput(LED_BASE, RED_LED);
  GPIOPinTypeGPIOInput(ButtonBase, Button);
  GPIOPadConfigSet(ButtonBase ,Button,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
  GPIOIntTypeSet(ButtonBase,Button,GPIO_FALLING_EDGE);
  GPIOIntRegister(ButtonBase,PortFIntHandler);
  GPIOIntEnable(ButtonBase, ButtonInt);
  
  /*
    Configures the Red LED, GPIO_PIN_1, to output
  */
  
  
  while(1){
	}
}
