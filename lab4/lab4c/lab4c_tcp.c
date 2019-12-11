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
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netdb.h>

#define B 4275
#define R0 100000.0

// temperature sensor
mraa_aio_context sensor;

char scale = 'F'; //defalut scale
int period = 1;	// default period
FILE * log_file = NULL;
int socket_fd;

int logging = 1;

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

	dprintf(socket_fd, "%s SHUTDOWN\n", time);
	
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

	char *time = get_time();

	dprintf(socket_fd, "%s %.1f\n", time, result);
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


int client_connect(char * host_name, unsigned int port){
	// DNS lookup
	struct sockaddr_in serv_addr;

	// TCP connection
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
		exit(1);
	}
	struct hostent *server = gethostbyname(host_name);
	if (server == NULL){
		fprintf(stderr, "Cannot find host: %s\n", host_name);
	}

	// convert host_name to IP address
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);


	// copy ip address from server to serv_addr
	serv_addr.sin_port = htons(port);
	int connection_status = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); 
	if (connection_status < 0){
		fprintf(stderr, "Failed to connect.\n");
	}
	return sockfd;

}

int main(int argc, char *argv[]) {
	int opt = 0;
	static struct option long_options [] = {
		{"scale", 	required_argument, 0, 's'},
		{"period",  required_argument, 0, 'p'},
		{"log", 	required_argument, 0, 'l'},
		{"id",		required_argument, 0, 'i'},
		{"host",	required_argument, 0, 'h'},
		{0, 			0 , 		   0 , 0 }
	};

	static char usage[] = "usage: ./lab4b --id=9-digit-number --host=name --log=filename.txt [--SCALE={F,C}] [--PERIOD=5] \n";

	int long_index = 0;

	char *id = "";
	char *host_name = "";

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
			/* ----- id ----- */	
			case 'i':
				id = optarg;
				break;
			/* ----- host ----- */
			case 'h':	
				host_name = optarg;
				break;
			default:
				fprintf(stderr, "Unrecognized argument.\n");
				printf("%s", usage);
				exit(1);
										
		}
	}

	if (log_file == NULL){
		fprintf(stderr, "Log file required.\n");
		exit(1);
	}

	if (strlen(id) != 9){
		fprintf(stderr, "9-digit id number required.\n");
		exit(1);
	}

	if (strlen(host_name) == 0){
		fprintf(stderr, "Hostname required.\n");
		exit(1);
	}

	int port = 0;
	if (argc - optind != 1){
		fprintf(stderr, "Invalid Port Number.\n");
		exit(1);
	}else {
		port = atoi(argv[optind]);
	}

	socket_fd = client_connect(host_name, port);

	// write id to socket
	dprintf(socket_fd, "ID=%s\n", id);
	fprintf(log_file, "ID=%s\n", id);
	
	sensor = mraa_aio_init(1);
	if (sensor == NULL){
		fprintf(stderr, "Failed to initialize sensor in AIO 1.\n");
		mraa_deinit();
		exit(1);
	}


	int temperature = 0;
	// handle the stdin during report generating by polling
	struct pollfd pollfds[1]; // only need to read from stdin
	pollfds[0].fd = socket_fd;
	pollfds[0].events = POLLIN;


	clock_t begin, end;
	begin = clock();
	end = (double)((clock() + period) * CLOCKS_PER_SEC);
	
	while (1){
		// start or stop writing report 
		if (logging && ((double)(end - begin)/CLOCKS_PER_SEC >= period)){
			//get_time();
			// read temperature from sensor
			temperature = mraa_aio_read(sensor);
			convert_temperature(temperature);
			begin = clock();
		}
		
		if (poll(pollfds, 1, 0) == -1){
			fprintf(stderr, "Poll Error\n");
			exit(1);
		}

		//  read from server
		if (pollfds[0].revents & POLLIN){
			// read from buffer
			char buffer[256];
			memset(buffer, 0, 256);
			
			if (read(socket_fd, buffer, 256) < 0){
				fprintf(stderr, "Failed to read server response.\n");
				exit(1);
			}
			// parse buffer to get commands and then execute
			process_command(buffer);
		}
		end = clock();
	}

	mraa_aio_close(sensor);
	if (log_file != NULL){
		fclose(log_file);
	}

	exit(0);
}
