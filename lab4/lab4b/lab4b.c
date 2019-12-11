#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <mraa/aio.h> //for sensor
#include <mraa/gpio.h> // for button
#include <math.h>
#include <time.h>

#define B 4275
#define R0 100000.0

// push button
mraa_gpio_context button;
// temperature sensor
mraa_aio_context sensor;

char scale = 'F'; //defalut scale
int period = 1;	// default period
FILE * log_file = NULL;

int button_pushed = 0;
int logging = 1;

// execution when button is pushed - log and exit
void do_when_pushed(){
	//button_pushed = 1;
	button_pushed = 1;
}

char* get_time(){
	char time[100];
	struct timespec ts;
	struct tm *tm;

	// time to report
	clock_gettime(CLOCK_REALTIME, &ts);
	tm = localtime(&(ts.tv_sec));
	
	sprintf(time, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	return strdup(&time[0]);
}

void shut_down(){
	// log and exit
	char *time = get_time();

	printf("%s SHUTDOWN\n", time);
	
	if (log_file != NULL){
		fprintf(log_file, "%s SHUTDOWN\n", time);
	}
	exit(0);
}

void convert_temperature(int reading){
	float R = 1023.0/((float) reading) - 1.0;
	R = R0 * R;
	float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
	float F = (C * 9)/5 + 32;

	float result;

	if (scale == 'C'){
		result = C;
	}else {
		result = F;
	}

	//return result;
	char *time = get_time();

	printf("%s %.1f\n", time, result);
	if (log_file != NULL){
		fprintf(log_file, "%s %.1f\n ",time, result);
	}
	
}


void process_command(char *buffer){
	if (log_file != NULL){
		fprintf(log_file, "%s\n", buffer);
	}

	if (strcmp(buffer, "SCALE=F\n") == 0){
		scale = 'F';
	}else if(strcmp(buffer, "SCALE=C\n") == 0){
		scale = 'C';
	}else if(strncmp(buffer, "PERIOD=", 7) == 0){
		int length = 0;
		
		for (int i = 7; buffer[i] != '\n'; i++){
			length++;
		}
		char temp[length];

		for (int i = 0; i < length; i++){
			temp[i] = buffer[7+i];
		}

		period = atoi(temp);
	}else if(strcmp(buffer, "START\n") == 0){
		logging = 1;
	}else if(strcmp(buffer, "STOP\n") == 0){
		logging = 0;
	}else if (strncmp(buffer, "LOG", 3) == 0){
		// No action but just log the entire command
		;
	}else if (strcmp(buffer, "OFF\n") == 0){
		shut_down();
	}else {
		fprintf(stderr, "Invalid command: %s\n", buffer);
		exit(1);
	}
}

int main(int argc, char *argv[]) {
	int opt = 0;
	static struct option long_options [] = {
		{"scale", 	required_argument, 0, 's'},
		{"period",  required_argument, 0, 'p'},
		{"log", 	required_argument, 0, 'l'},
		{0, 			0 , 		   0 , 0 }
	};

	static char usage[] = "usage: ./lab4b [--SCALE=[F,C]] [--PERIOD=5] [--STOP] [START] [LOG file.txt] [OFF]\n";

	int long_index = 0;

	while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) != -1){
		switch (opt){
			/* ----- scale ------ */
			case 's':
				if (optarg[0] == 'F'){
					scale = 'F';
				}else if (optarg[0] == 'C'){
					scale = 'C';
				}else{
					fprintf(stderr, "Unsupported option for scale.\n");
					exit(1);
				}
				break;
			/* ----- period ------ */	
			case 'p':
				period = atoi(optarg);
				break;
			/* ----- log ------ */	
			case 'l':
				// open a log file 
				if ((log_file = fopen(optarg, "a"))== NULL){
					fprintf(stderr, "Cannot open log file %s.\n", optarg);
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Unrecognized argument.\n");
				printf("%s", usage);
				exit(1);
										
		}
	}

	// initialize push button and sensor
	button = mraa_gpio_init(60);
	if (button == NULL){
		fprintf(stderr, "Failed to initialize button in GPIO 60.\n");
		mraa_deinit();
		exit(1);
	}
	
	sensor = mraa_aio_init(1);
	if (sensor == NULL){
		fprintf(stderr, "Failed to initialize sensor in AIO 1.\n");
		mraa_deinit();
		exit(1);
	}

	// set GPIO to input
	int status = mraa_gpio_dir(button, MRAA_GPIO_IN);
	if (status != MRAA_SUCCESS){
		fprintf(stderr, "Failed to set button in GPIO 60 to input.\n");
		exit(1);
	}

	status = mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &do_when_pushed, NULL);
	if (status != MRAA_SUCCESS){
		fprintf(stderr, "Failed to set an interrupt.\n");
		exit(1);
	}

	int temperature = 0;
	// handle the stdin during report generating by polling
	struct pollfd pollfds[1]; // only need to read from stdin
	pollfds[0].fd = STDIN_FILENO;
	pollfds[0].events = POLLIN;


	clock_t begin, end;
	begin = clock();
	end = (double)((clock() + period) * CLOCKS_PER_SEC);
	
	while (1){
		// start or stop writing report 
		if (logging && ((double)(end - begin)/CLOCKS_PER_SEC >= period)){
			// read temperature from sensor
			temperature = mraa_aio_read(sensor);
			convert_temperature(temperature);
			begin = clock();
		}
		
		if (poll(pollfds, 1, 0) == -1){
			fprintf(stderr, "Poll Error\n");
			exit(1);
		}

		// stdin is readable
		if (pollfds[0].revents & POLLIN){
			// read from buffer
			char buffer[256];
			memset(buffer, 0, 256);
			if (fgets(buffer, 256, stdin) == NULL){
				fprintf(stderr, "Cannot read commands from stdin.\n");
				exit(1);
			}
			// parse buffer to get commands and then execute
			process_command(buffer);
		}

		if (button_pushed){
			shut_down();
		}


		end = clock();
	}

	mraa_gpio_close(button);
	mraa_aio_close(sensor);
	if (log_file != NULL){
		fclose(log_file);
	}

	exit(0);
}
