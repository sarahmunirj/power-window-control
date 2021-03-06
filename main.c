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
#define BLUE_LED 				GPIO_PIN_2
#define GREEN_LED				GPIO_PIN_3

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

#define MOT1_PERIPH			SYSCTL_PERIPH_GPIOA
#define MOT1_BASE 			GPIO_PORTA_BASE
#define MOT1_A 					GPIO_PIN_4
#define MOT1_B 					GPIO_PIN_3

#define STOP_PERIPH			SYSCTL_PERIPH_GPIOB
#define STOP_BASE				GPIO_PORTB_BASE
#define OBSTACLE				GPIO_PIN_6
#define ENDSTOP					GPIO_PIN_7
#define OBSTACLE_INT		GPIO_INT_PIN_6
#define ENDSTOP_INT			GPIO_INT_PIN_7

uint32_t SystemCoreClock = 16000000;
#define FULL_UP_DOWN_US 5000
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
	bool obstacle;
} inputs;

volatile uint32_t counter = 0;
volatile uint32_t last_up = 0;
volatile uint32_t last_down = 0;
volatile bool full_up=0;
volatile bool full_down=0;
volatile state State;
volatile inputs Inputs;

void Button_Handler(void);
void Passenger_Up(void);
void Passenger_Down(void);

void Passenger_Up(void){
		GPIOPinWrite(LED_BASE,RED_LED, RED_LED);
		GPIOPinWrite(LED_BASE,BLUE_LED, 0);
		GPIOPinWrite(MOT1_BASE, MOT1_A, MOT1_A);
		GPIOPinWrite(MOT1_BASE, MOT1_B, 0);
}

void Passenger_Down(void){
		GPIOPinWrite(LED_BASE,BLUE_LED, BLUE_LED);
		GPIOPinWrite(LED_BASE,RED_LED, 0);
		GPIOPinWrite(MOT1_BASE, MOT1_B, MOT1_B);
		GPIOPinWrite(MOT1_BASE, MOT1_A, 0);
}

void Passenger_Stop(void){
		GPIOPinWrite(LED_BASE,RED_LED, 0);
		GPIOPinWrite(LED_BASE,BLUE_LED, 0);
		GPIOPinWrite(MOT1_BASE, MOT1_A, 0);
		GPIOPinWrite(MOT1_BASE, MOT1_B, 0);
}

void SysTickIntHandler(void){
    //
    // Update the Systick interrupt counter.
    //
    counter++;
}

  
void Button_Handler(void){	
	uint32_t status=0;
	uint32_t value=0;

	ROM_SysCtlDelay(30000);
  status = GPIOIntStatus(BUTT_BASE,true);
  GPIOIntClear(BUTT_BASE,GPIO_ALL_PINS);
  if((status & DRIVER_UP_INT)==DRIVER_UP_INT)
	{
		value = GPIOPinRead(BUTT_BASE, DRIVER_UP);
		if ( value == 0 ) 
		{
			Inputs.driver_up = 1;
			last_up = counter;
		}
		else if (counter - last_up <= 1000)
		{
			full_up = 1;
			Inputs.driver_up = 1;
			last_up = 0;
		}
		else 
		{
		Inputs.driver_up = 0;
		}
	}
	if((status & DRIVER_DOWN_INT)==DRIVER_DOWN_INT)
	{
		value = GPIOPinRead(BUTT_BASE, DRIVER_DOWN);
		if (value == 0)
		{
			Inputs.driver_down = 1;
			last_down = counter;
		}
		else if (counter - last_down <= 1000)
		{
			full_down = 1;
			Inputs.driver_down = 1;
			last_down = 0;
		}
		else 
		{	
			Inputs.driver_down = 0;
		}
	}
  if((status & PASS_UP_INT) == PASS_UP_INT)
	{
		value = GPIOPinRead(BUTT_BASE, PASS_UP);
		if (value == 0)
		{
			Inputs.pass_up = 1;
			last_up = counter;
		}
		else if (counter - last_up <= 1000)
		{
			full_up = 1;
			Inputs.pass_up = 1;
			last_up = 0;
		}
		else 
		{
			Inputs.pass_up = 0;
		}
	}
  if((status & PASS_DOWN_INT) == PASS_DOWN_INT)
	{
		value = GPIOPinRead(BUTT_BASE, PASS_DOWN);
		if (value == 0)
		{
			Inputs.pass_down = 1;
			last_down = counter;
		}
		else if ((counter - last_down <= 1000))
		{
			full_down = 1;
			Inputs.pass_down = 1;
			last_down = 0;
		}
				else 
		{
			Inputs.pass_down = 0;
		}
	}
}



void Stop_Handler(void)
{
	uint32_t status=0;
	uint32_t value=0;

	ROM_SysCtlDelay(30000);
  status = GPIOIntStatus(STOP_BASE,true);
  GPIOIntClear(STOP_BASE,GPIO_ALL_PINS);
	if((status & OBSTACLE)==OBSTACLE_INT)
	{
		value = GPIOPinRead(STOP_BASE, OBSTACLE);
		if ( value == 0 ) 
		{
			Inputs.obstacle = 1;
		}
		else 
		{
		Inputs.obstacle = 0;
		}
	}
	if((status & ENDSTOP)==ENDSTOP_INT)
	{
		value = GPIOPinRead(STOP_BASE, ENDSTOP);
		if ( value == 0 ) 
		{
			Inputs.endswitch = 1;
		}
		else 
		{
		Inputs.endswitch = 0;
		}
	}
}


void system_init(void)
	{
	ROM_SysCtlPeripheralEnable(LED_PERIPH);
	ROM_SysCtlDelay(3);
	ROM_SysCtlPeripheralEnable(BUTT_PERIPH);
	ROM_SysCtlDelay(3);
	ROM_SysCtlPeripheralEnable(MOT1_PERIPH);
	ROM_SysCtlDelay(3);
	ROM_SysCtlPeripheralEnable(STOP_PERIPH);
	ROM_SysCtlDelay(3);
		
	ROM_GPIOPinTypeGPIOOutput(LED_BASE, RED_LED | BLUE_LED);
	ROM_GPIOPinTypeGPIOOutput(MOT1_BASE, MOT1_A | MOT1_B);
	ROM_GPIOPinTypeGPIOInput(BUTT_BASE, PASS_DOWN | PASS_UP | DRIVER_DOWN | DRIVER_UP);
	ROM_GPIOPinTypeGPIOInput(STOP_BASE, OBSTACLE | ENDSTOP);

		
	ROM_GPIOPadConfigSet(BUTT_BASE, PASS_DOWN | PASS_UP | DRIVER_DOWN | DRIVER_UP, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);// set all switches to be pulledUp
	ROM_GPIOPadConfigSet(STOP_BASE, OBSTACLE | ENDSTOP, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);// set all switches to be pulledUp
	
	GPIOIntDisable(BUTT_BASE, GPIO_ALL_PINS);
	GPIOIntDisable(STOP_BASE, GPIO_ALL_PINS);
		
	GPIOIntClear(BUTT_BASE, GPIO_ALL_PINS); 
	GPIOIntClear(STOP_BASE, GPIO_ALL_PINS); 

	GPIOIntTypeSet(BUTT_BASE, PASS_DOWN | PASS_UP | DRIVER_DOWN | DRIVER_UP ,  GPIO_BOTH_EDGES); 
	GPIOIntTypeSet(STOP_BASE, OBSTACLE | ENDSTOP,  GPIO_FALLING_EDGE); 

	GPIOIntRegister(BUTT_BASE,Button_Handler);
	GPIOIntRegister(STOP_BASE,Stop_Handler);

	GPIOIntEnable(BUTT_BASE, PASS_DOWN_INT | PASS_UP_INT | DRIVER_DOWN_INT | DRIVER_UP_INT);
	GPIOIntEnable(STOP_BASE, OBSTACLE_INT | ENDSTOP_INT);

//	ROM_IntPrioritySet(INT_GPIOB_TM4C123, 2);
//	ROM_IntPrioritySet(INT_GPIOC_TM4C123, 3);

	SysTickDisable();
	SysTickPeriodSet((SystemCoreClock/3)/1000);
	SysTickIntRegister(SysTickIntHandler);
	ROM_IntMasterEnable();
	SysTickIntEnable();
	SysTickEnable();
}
	
int main()
{
	system_init();
	uint32_t full_counter =0;
	State.CurrentState = safe_state;
	State.CurrentDriverState = driver_neutral;
	State.CurrentPassState = pass_neutral;
	Inputs.safe_lock = 0;
	Inputs.obstacle = 0;
	Inputs.endswitch = 0;
	

	while(1)
	{		
		if(!Inputs.obstacle)
		{
			State.CurrentState = safe_state;
			if(!Inputs.endswitch)
			{
				if(Inputs.driver_up)
				{
					if(full_up == 1)
					{
						Passenger_Up();
						full_up = 0;
						Inputs.driver_up = 0;
						full_counter = counter;
						while(counter - full_counter <= FULL_UP_DOWN_US)
						{
							if(Inputs.endswitch |Inputs.obstacle)
								break;
						}
						Passenger_Stop();
						State.CurrentDriverState = driver_neutral;
					}
					else if(State.CurrentDriverState != driver_up)
					{
						State.CurrentDriverState = driver_up;
					}
					else 
						Passenger_Up();
				}
				else if(Inputs.driver_down)	
				{
					if(full_down == 1)
					{
						Passenger_Down();
						full_down = 0;
						Inputs.driver_down = 0;
						full_counter = counter;
						while(counter - full_counter <= FULL_UP_DOWN_US)
						{
							if(Inputs.endswitch |Inputs.obstacle)
								break;
						}
						Passenger_Stop();
						State.CurrentDriverState = driver_neutral;
					}				
					else if(State.CurrentDriverState != driver_down)
					{
						State.CurrentDriverState = driver_down;
					}
					else 
						Passenger_Down();
				}
				else
				{
					State.CurrentDriverState = driver_neutral;
					if (Inputs.safe_lock == 0)
					{					
						if(Inputs.pass_up)
						{
							if(full_up == 1)
							{
								Passenger_Up();
								full_up = 0;
								Inputs.pass_up = 0;
								full_counter = counter;
								while(counter - full_counter <= FULL_UP_DOWN_US){
									if(Inputs.driver_down | Inputs.driver_up |Inputs.endswitch |Inputs.obstacle)
										break;
								}								
								Passenger_Stop();
								State.CurrentPassState = pass_neutral;
							}
							else if(State.CurrentPassState != pass_up)
							{
								State.CurrentPassState = pass_up;
									Passenger_Up();
							}
						}
						else if(Inputs.pass_down)
						{
							if(full_down == 1)
							{
								Passenger_Down();
								full_down = 0;
								Inputs.pass_down = 0;
								full_counter = counter;
								while(counter - full_counter <= FULL_UP_DOWN_US){
									if(Inputs.driver_down | Inputs.driver_up |Inputs.endswitch |Inputs.obstacle)
										break;
								}
								Passenger_Stop();
								State.CurrentPassState = pass_neutral;
							}
							else if(State.CurrentPassState != pass_down)
							{
								State.CurrentPassState = pass_down;
								Passenger_Down();
							}
						}
						else
						{
							State.CurrentPassState = pass_neutral;
							Passenger_Stop();
						}
					}
					else Passenger_Stop();
				}
			}
			else
			Passenger_Stop();
			Inputs.endswitch = 0;
		}
		else
		{
			State.CurrentState = emergency_down;
			Inputs.obstacle = 0;
			Passenger_Down();
			full_counter = counter;
			while(counter - full_counter <= FULL_UP_DOWN_US){	}
			Passenger_Stop();
			State.CurrentState = safe_state;
		}	
	}
}

