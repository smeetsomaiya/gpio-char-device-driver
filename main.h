

/*
Structures to read input from the file
*/
struct task_params_struct
{
	int duty_cycle;
	int red_led;
	int green_led;
	int blue_led;
	int on_time;
	int off_time;
	int intensity_control_period_ms;
	int step_period_ms;
};

#define MAX_PIN_NUMBER 19
struct task_params_struct read_input_file();

// Mouse functionality
pthread_t mouse_thread, log_thread;
bool mouse_failed;
inline int create_mouse_thread();
void* mouse_thread_body(void *arguments);
void exit_signal_handler(int signum);

inline int IOSetup(struct task_params_struct *pin_data);
inline int calculate_on_time(float duty_cycle);	
inline int calculate_off_time(float duty_cycle);

//Macros related to lightup sequence
#define STEP_PERIOD_MS 500
#define INTENSITY_CONTROL_PERIOD_MS 20

inline void mouse_event_handler(int event);

int led_file_desc; //Led file descriptor

//Array containing the sequence
int lightup_pattern_array[8][3] = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {1, 1, 0},
        {1, 0, 1},
        {0, 1, 1},
        {1, 1, 1},
};

int sequence_array[8] = {0, 4, 2, 1, 6, 5, 3, 7};

#define DRIVER_PATH "/dev/RGBLed"

int exit_flag = 0;

#define ROW_SIZE(arr) ((int) (sizeof (arr) / sizeof (arr)[0]))
#define COLUMN_SIZE(arr) ((int) sizeof ((arr)[0]) / sizeof (int))
