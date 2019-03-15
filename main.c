#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include<unistd.h>
#include<signal.h>
#include<stdbool.h>
#include<errno.h>
#include "main.h"

#define CONFIG 11
#define RESET 12

int main(int argc, char **argv)
{
	int err;
	struct task_params_struct input_data = read_input_file();

	signal(SIGUSR1, exit_signal_handler); //Set the exit signal handler on the mouse thread
	printf("\n\nInput Data\n Duty cycle %d\n", input_data.duty_cycle);

	if(IOSetup(&input_data)) { // Configure the device driver params
		goto exit;
	}

	create_mouse_thread();
	write(led_file_desc, (char*)&sequence_array, ROW_SIZE(sequence_array)); //Start the lighting sequence
	
	sleep(15); //Sleep for the time you want the pattern to execute

	if(!mouse_failed) {
		printf("Waiting for mouse thread to exit\n");
		pthread_kill(mouse_thread, SIGUSR1);
		pthread_join(mouse_thread, NULL);        //waits for mouse thread and log thread to exit
	}
	exit:
	exit_flag = 1;
	err = close(led_file_desc);
	printf("\nExiting %d\n", err);

	exit(0);
}

struct task_params_struct read_input_file() {//Structure to read the input file including duty_cycle and three IO pins connected to R,G,B Leds respectively
	struct task_params_struct params;
	char *c = NULL;
	ssize_t nread;
	size_t len = 0;
	FILE *input_file;

	input_file = fopen("input.txt", "r");                     //open the input text file
	while((nread = getline(&c,&len,input_file)) != -1) {
		params.duty_cycle = atoi(strsep(&c, ","));
		params.red_led = atoi(strsep(&c, ","));
		params.green_led = atoi(strsep(&c, ","));
		params.blue_led = atoi(strsep(&c, ","));
		printf("Duty cycle - %d\n", params.duty_cycle);
		printf("Red led is on IO%d\n", params.red_led);
		printf("Green led is on IO%d\n", params.green_led);
		printf("Blue led is on IO%d\n", params.blue_led);
		
		params.on_time = calculate_on_time(params.duty_cycle);
		params.off_time = calculate_off_time(params.duty_cycle);
		params.step_period_ms = (int) STEP_PERIOD_MS;
		params.intensity_control_period_ms = (int) INTENSITY_CONTROL_PERIOD_MS;
		
		printf("On time %d ms\n", params.on_time);
		printf("Off time %d ms\n", params.off_time);
	}
	fclose(input_file);                                       //close the input text file
	return params;
}

int create_mouse_thread()
{
	int err = pthread_create(&mouse_thread, NULL, &mouse_thread_body, NULL);//Creates a mouse thread to continuously poll for an event
	if (err != 0) {
		printf("\nCan't create mouse thread :[%s]", strerror(err));
	}
	else {
		printf("\nMouse Thread created successfully\n");
    	}
	return err;
}

void* mouse_thread_body(void *arguments)
{     
	struct input_event ie;
	int fd,bytes, mclick;
	const char* MOUSE_DEV = getenv("MOUSE_DEV");
	printf("Trying to open mouse device %s\n", MOUSE_DEV);
	if((fd = open(MOUSE_DEV, O_RDONLY)) == -1)
	{
		mouse_failed = true;
		perror("opening mouse device");
		printf("Mouse thread exit\n");
		pthread_exit(NULL);
		return NULL;
	}
	else {
		printf("mouse file opened successfully%s\n", MOUSE_DEV);
	}

	while(exit_flag == 0)
	{
		bytes = read(fd, &ie, sizeof(struct input_event));
		if(bytes > 0)
		{
			if(ie.type== EV_KEY && ie.code == BTN_RIGHT && ie.value==0)
			{
				mclick = ie.code;
				printf("Right Mouse is clicked\n");
				mouse_event_handler(mclick);
			}
			else if(ie.type== EV_KEY && ie.code == BTN_LEFT && ie.value==0)
			{
				mclick = ie.code;
				printf("Left Mouse is clicked\n");    
				mouse_event_handler(mclick);    
			}
		}
	}

	printf("mouse thread exiting");
	pthread_exit(NULL);
	return NULL;
}

/*
 * A signal handler that handles the signal received from the mouse event so as to immmediately stop the current running sequence and roll back to initial state
 */
void mouse_event_handler(int event)
{	
	int err;
	err = ioctl(led_file_desc, (unsigned int)RESET, NULL); //restart the sequence
	if(err < 0) {
		printf("\nFailed to restart sequence\n");
	} else {
		printf("\nSuccessfully restarted sequence\n");	
	}

}

void exit_signal_handler(int signum) {
	if(signum == SIGUSR1) {
		printf("Received exit signal %d\n", signum);
		pthread_cancel(mouse_thread);
	}
}

/*
 * Setting up IO to map each input IO pin to the internally connected gpio from the array
 */
int IOSetup(struct task_params_struct *pin_data)
{
	int err;
	led_file_desc = open(DRIVER_PATH, O_RDWR);
	if(led_file_desc < 0) {
		printf("\nError opening device - %s\n", strerror(led_file_desc));
		return -1;
	}
	printf("\nDevice opened %s %d\n", DRIVER_PATH, led_file_desc);
	err = ioctl(led_file_desc, (unsigned int)CONFIG, (unsigned long)pin_data); //Configure the leds
	if(err == EINVAL) {
		return -1;
	}
	return 0;
}

/*
 *Calculates the Led on time
*/
int calculate_on_time(float duty_cycle) 
{                                                
	float percent_value = ( (float) duty_cycle) / 100.0;      //Converts the input percent value of duty cycle
	printf("Percent value for on time %f\n", percent_value);
	printf("Intensity control period %d ms\n", INTENSITY_CONTROL_PERIOD_MS);
	return (int) ((percent_value) * ((int) INTENSITY_CONTROL_PERIOD_MS)); //Returns the Calculated led on time
}
/*
 *Calculates the Led off time
*/
int calculate_off_time(float duty_cycle) 
{                                                //Calculates the Led on time
	float percent_value = 1 - (((float) duty_cycle) / 100.0);      //Converts the input percent value of duty cycle
	printf("Percent value for off time %f\n", percent_value);
	printf("Intensity control period %d ms\n", INTENSITY_CONTROL_PERIOD_MS);
	return (int) ((percent_value) * INTENSITY_CONTROL_PERIOD_MS); //Returns the Calculated led on time
}
