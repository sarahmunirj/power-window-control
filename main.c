//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include <task.h>


#define GPIO_ALL_PINS 	0xFF

#define LED_PERIPH 			SYSCTL_PERIPH_GPIOF
#define LED_BASE 				GPIO_PORTF_BASE
#define RED_LED 				GPIO_PIN_1

#define BUTT_PERIPH 		SYSCTL_PERIPH_GPIOC
#define BUTT_BASE 			GPIO_PORTC_BASE
#define PASS_UP 				GPIO_PIN_4
#define PASS_DOWN 			GPIO_PIN_5	
#define DRIVER_UP 			GPIO_PIN_6
#define DRIVER_DOWN 		GPIO_PIN_7	
#define PASS_UP_INT 				GPIO_INT_PIN_4
#define PASS_DOWN_INT 			GPIO_INT_PIN_5	
#define DRIVER_UP_INT 			GPIO_INT_PIN_6
#define DRIVER_DOWN_INT 		GPIO_INT_PIN_7	


#define MOT1_PERIPH			SYSCTL_PERIPH_GPIOD
#define MOT1_BASE 			GPIO_PORTD_BASE
#define MOT1_A 					GPIO_PIN_6
#define MOT1_B 					GPIO_PIN_7

//Define system clock

enum pass_state_e{
    pass_neutral,
    pass_down,
    pass_up
};

enum driver_state_e{
    driver_neutral,
    driver_down,
    driver_up
};

enum state_e{
    safe_state,
	emergency_down
};

typedef struct state {
    enum state_e CurrentState;
    enum driver_state_e CurrentDriverState;
    enum pass_state_e CurrentPassState;
} state;

typedef struct inputs{
    bool pass_up;
    bool pass_down;
    bool driver_up;
    bool driver_down;
    bool endswitch; 
    bool safe_lock;
} inputs;

//typedef struct outputs{
//    bool move_up;
//    bool move_down;
//} outputs;



uint32_t SystemCoreClock = 16000000;
volatile uint32_t counter = 0;
volatile uint32_t previous_counter = 0;
volatile uint32_t last_up=0;
volatile state State;
volatile inputs Inputs;

void Button_Handler(void);
void Passenger_Up(void);
void Passenger_Down(void);


void Passenger_Up(void){
		GPIOPinWrite(LED_BASE,RED_LED, RED_LED);
		GPIOPinWrite(MOT1_BASE, MOT1_A, MOT1_A);
		GPIOPinWrite(MOT1_BASE, MOT1_B, 0);
}

void Passenger_Down(void){
		GPIOPinWrite(LED_BASE,RED_LED, RED_LED);
		GPIOPinWrite(MOT1_BASE, MOT1_B, MOT1_B);
		GPIOPinWrite(MOT1_BASE, MOT1_A, 0);
}

void Passenger_Stop(void){
		GPIOPinWrite(LED_BASE,RED_LED, 0);
		GPIOPinWrite(MOT1_BASE, MOT1_A, 0);
		GPIOPinWrite(MOT1_BASE, MOT1_B, 0);
}

void
SysTickIntHandler(void)
	{
    //
    // Update the Systick interrupt counter.
    //
    counter++;
}

void Button_Handler(void)
{	
	uint32_t status=0;
	ROM_SysCtlDelay(50000);
  status = GPIOIntStatus(BUTT_BASE,true);
  GPIOIntClear(BUTT_BASE,status);

	
  if(status & DRIVER_UP_INT)
		Inputs.driver_up = GPIOPinRead(BUTT_BASE, GPIO_PIN_4);
    //Then there was a driver up pin interrupt
  if(status & DRIVER_DOWN_INT)
    //Then there was a driver up pin interrupt
    Inputs.driver_down = GPIOPinRead(BUTT_BASE, GPIO_PIN_5);
   if(status & PASS_UP_INT)
    //Then there was a driver up pin interrupt
    Inputs.pass_up = GPIOPinRead(BUTT_BASE, GPIO_PIN_6);
  if(status & PASS_DOWN_INT)
    //Then there was a driver up pin interrupt
    Inputs.pass_down = GPIOPinRead(BUTT_BASE, GPIO_PIN_7); 
		
//    if( value==0)
//		{
//			Passenger_Up();
//			//state time
//			last_up = counter;
//		}
//		else
//		{
//			//if time between last up and this down is less than 500 ms then Driver up 
//			//else driver stop
//			if (counter - last_up <= 500)
//			{
//				Passenger_Up();
//				ROM_SysCtlDelay(10000000);
//				Passenger_Stop();
//				last_up = 0;
//			}
//			else 
//				Passenger_Stop();
//		}
//	}		

}






void system_init(void)
	{
	ROM_SysCtlPeripheralEnable(LED_PERIPH);
	ROM_SysCtlDelay(1);
	ROM_SysCtlPeripheralEnable(BUTT_PERIPH);
	ROM_SysCtlDelay(1);
	ROM_SysCtlPeripheralEnable(MOT1_PERIPH);
	ROM_SysCtlDelay(1);

	ROM_GPIOPinTypeGPIOOutput(LED_BASE, RED_LED);
	ROM_GPIOPinTypeGPIOOutput(MOT1_BASE, MOT1_A | MOT1_B);
	ROM_GPIOPinTypeGPIOInput(BUTT_BASE, PASS_DOWN);
	ROM_GPIOPinTypeGPIOInput(BUTT_BASE, PASS_UP);
	ROM_GPIOPinTypeGPIOInput(BUTT_BASE, DRIVER_DOWN);
	ROM_GPIOPinTypeGPIOInput(BUTT_BASE, DRIVER_UP);


	ROM_GPIOPadConfigSet(BUTT_BASE, PASS_DOWN | PASS_UP | DRIVER_DOWN | DRIVER_UP, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);// set all switches to be pulledUp
		
	GPIOIntDisable(BUTT_BASE, GPIO_ALL_PINS);
	GPIOIntClear(BUTT_BASE, GPIO_ALL_PINS); 
	GPIOIntTypeSet(BUTT_BASE, PASS_DOWN | PASS_UP | DRIVER_DOWN | DRIVER_UP ,  GPIO_BOTH_EDGES); // Configure PF4 for falling edge trigger
	GPIOIntRegister(BUTT_BASE,Button_Handler);
	GPIOIntEnable(BUTT_BASE, GPIO_ALL_PINS);
	//ROM_IntPrioritySet(INT_GPIOF_TM4C123, 3);

	SysTickDisable();
	SysTickPeriodSet(SystemCoreClock/1000);
	SysTickIntRegister(SysTickIntHandler);
	ROM_IntMasterEnable();
	SysTickIntEnable();
	SysTickEnable();
}
	
int main()
{
		State.CurrentState = safe_state;
		State.CurrentDriverState = driver_neutral;
		State.CurrentPassState = pass_neutral;

    Inputs.safe_lock = 0;
    while(1)
		{
			switch(State.CurrentState){
					case safe_state:
					{
							switch(State.CurrentDriverState)
							{
									case driver_up:
											Passenger_Up();
											break;
									case driver_down:
											Passenger_Down();
											break;
									case driver_neutral:
											if (Inputs.safe_lock == 0)
											{
													switch(State.CurrentPassState){
															case pass_up:
																	Passenger_Up();
																	break;
															case pass_down:
																	Passenger_Down();
																	break;
															case pass_neutral:
																	Passenger_Stop();
															break;
													}
											}
											Passenger_Stop();
											break;
									default:
											Passenger_Stop();
							}
							break;
					}
					case emergency_down:
					Passenger_Down();
					ROM_SysCtlDelay(10000000);
			}
    }
}

