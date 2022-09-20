#include <stdio.h>
#include "platform.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xgpio.h"
#include "xil_types.h"
#include "time.h"

// Get device IDs from xparameters.h
#define CTRL_ID XPAR_AXI_GPIO_MOTOR_CONTROL_DEVICE_ID
#define TRIG_ID XPAR_AXI_GPIO_TRIGGER_DEVICE_ID
#define ECHO_ID XPAR_AXI_GPIO_ECHO_DEVICE_ID
#define CTRL_CHANNEL 1
#define TRIG_CHANNEL 1
#define ECHO_CHANNEL 1

// Timer initialization function

XTmrCtr tmr;
void tmr_init(){
	 int status =  XTmrCtr_Initialize(&tmr,XPAR_AXI_TIMER_0_DEVICE_ID );

 	 //if(status == XST_SUCCESS)
 		 //xil_printf("TMR INIT SUCCESSUFUL\n");
 	 //else
 		 //xil_printf("TMR INIT FAILED\n");

 	 status = XTmrCtr_SelfTest(&tmr, 0);

 	 //if(status == XST_SUCCESS)
 		 //xil_printf("TMR SELFTEST SUCCESSUFUL\n");
 	 //else
 		 //xil_printf("TMR SELFTEST FAILED\n");
}

void wait(int nanoseconds){

	u32 wait = nanoseconds;
	init_platform();

	XTmrCtr_Stop(&tmr, 0);
	XTmrCtr_SetResetValue(&tmr, 0, wait);

	XTmrCtr_Reset(&tmr, 0);

	u32 option = XTmrCtr_GetOptions(&tmr, 0);
	XTmrCtr_SetOptions(&tmr, 0, option | XTC_DOWN_COUNT_OPTION );

	XTmrCtr_Start(&tmr, 0);
	while(!XTmrCtr_IsExpired(&tmr, 0));

}

void send_trig_pulse(){

	XGpio_Config *cfg_ptr;
	XGpio trig_device;

	// Initialize Trigger Device
	cfg_ptr = XGpio_LookupConfig(TRIG_ID);
	XGpio_CfgInitialize(&trig_device, cfg_ptr, cfg_ptr->BaseAddress);

	// Set Trigger Tristate (Output)
	XGpio_SetDataDirection(&trig_device, TRIG_CHANNEL, 0);

	//Sending 10 us pulse
	XGpio_DiscreteWrite(&trig_device, TRIG_CHANNEL, 1);
	wait(60);
	XGpio_DiscreteWrite(&trig_device, TRIG_CHANNEL, 0);

	//xil_printf("Pulse sent\n");

	cleanup_platform();
}

int get_distance(){

	XGpio_Config *cfg_ptr;
	XGpio echo_device;
	u32 echo_state;
	int count = 0;
	int count2 = 0;
	u32 echo_pulse_started = 0;
	int distance = 333;

	// Initialize Echo Device
	cfg_ptr = XGpio_LookupConfig(ECHO_ID);
	XGpio_CfgInitialize(&echo_device, cfg_ptr, cfg_ptr->BaseAddress);

	// Set Echo Tristate (Input)
	XGpio_SetDataDirection(&echo_device, TRIG_CHANNEL, 1);


	while (1){
		//xil_printf("Stuck in loop 1");
		echo_state = XGpio_DiscreteRead(&echo_device, ECHO_CHANNEL);
		if (echo_state){
			wait(5000000);
		}
		else{
			send_trig_pulse();
			wait(60);
			while(1){
				//xil_printf("Stuck in loop 2");
				echo_state = XGpio_DiscreteRead(&echo_device, ECHO_CHANNEL);
				//xil_printf("echo_state: %d\n", echo_state);

				if (echo_state){
					echo_pulse_started = 1;
					count += 1;
					//xil_printf("count: %d\n", count);
				}
				if (count2 >= 1250) {
					distance = 3333;
					break;
				}
				if ((echo_pulse_started) && (!echo_state)){

					distance = (((count) * 343)/2/100); // distance in mm
					//xil_printf("%d\n", (int)(two));
					break;
				}
				count2 += 1;
				wait(60);
				//xil_printf("count: %d\n", distance);
			}
		}
		//xil_printf("Broke out");
		break;
	}
	return distance;
}

void move_forward_or_reverse(unsigned char forward_or_reverse){
	XGpio_Config *cfg_ptr;
	XGpio ctrl_device;

	// Initialize Control Device
	cfg_ptr = XGpio_LookupConfig(CTRL_ID);
	XGpio_CfgInitialize(&ctrl_device, cfg_ptr, cfg_ptr->BaseAddress);

	// Set Control Tristate (Output)
	XGpio_SetDataDirection(&ctrl_device, CTRL_CHANNEL, 0);

	if (forward_or_reverse){
		XGpio_DiscreteWrite(&ctrl_device, CTRL_CHANNEL, 0b01111);
	}
	else{
		XGpio_DiscreteWrite(&ctrl_device, CTRL_CHANNEL, 0b10111);
	}
}

void turn(unsigned char right_or_left){
	XGpio_Config *cfg_ptr;
	XGpio ctrl_device;

	// Initialize Control Device
	cfg_ptr = XGpio_LookupConfig(CTRL_ID);
	XGpio_CfgInitialize(&ctrl_device, cfg_ptr, cfg_ptr->BaseAddress);

	// Set Control Tristate (Output)
	XGpio_SetDataDirection(&ctrl_device, CTRL_CHANNEL, 0);

	if (right_or_left){
		XGpio_DiscreteWrite(&ctrl_device, CTRL_CHANNEL, 0b00111);
	}
	else{
		XGpio_DiscreteWrite(&ctrl_device, CTRL_CHANNEL, 0b11111);
	}
}

void stop(unsigned char stop){
	XGpio_Config *cfg_ptr;
	XGpio ctrl_device;

	// Initialize Control Device
	cfg_ptr = XGpio_LookupConfig(CTRL_ID);
	XGpio_CfgInitialize(&ctrl_device, cfg_ptr, cfg_ptr->BaseAddress);

	// Set Control Tristate (Output)
	XGpio_SetDataDirection(&ctrl_device, CTRL_CHANNEL, 0);

	if (stop){
		XGpio_DiscreteWrite(&ctrl_device, CTRL_CHANNEL, 0b00000);
	}
}

int main() {

int distance_measured;
tmr_init();

	while (1) {
		//send_trig_pulse();
		distance_measured = get_distance();
		//xil_printf("Distance: %d\n", distance_measured);
		if (distance_measured <= 300){
			turn(1);
			wait(100000000);
		}
		else{
			move_forward_or_reverse(1);
		}
		//wait(100000000);//2 second delay
	}
	return 0;
}
