/*
*	prog3_server.c
*	CSCI 367
*  
*	Description: The server of the program that will run and wait for participants and
*					Obsersers clients to join / disconnect at any moment. Panticipants 
* 					can send either public / private messages to other participants.
*  
*	Sereyvathanak Khorn
* 	
*/

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/types.h>
#include <math.h>

#define QLEN 6 /* size of request queue */
#define par 255
#define obs 255
#define MAX 255

typedef struct {
	int sd;						// its socket
	int obs_sd;					// Obs socket (Store in the index to the observer)
	char username[10];  		// Username
	struct timeval coolDown;	// Timeout for entering username
} par_sock;

typedef struct {
	int sd;
	int par_sd;
	char username[10];
	struct timeval coolDown;
} obs_sock;

struct timeval time = {0,0};
struct timeval minTime = {0,0};
struct timeval prevTime = {0,0};

par_sock par_array[MAX];
obs_sock obs_array[MAX];
bool pending = false;
int timeInSec = 60;

bool checkMessage(char* message);
void printAll(char* message, int nlen);
void timeup (par_sock par_array[], obs_sock obs_array[]);
bool checkPending (par_sock par_array[], obs_sock obs_array[]);
void initClientSpace();
void DCPar(par_sock par_array[], int i, int par_array_size);
bool validateName(char* buf, int nlen);

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd; /* socket descriptors */
	int sd2; /* socket descriptors */
	// int port; /* protocol port number */
	int alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */
	char buf[10]; /* buffer for string the server sends */
	uint16_t port;
	uint16_t port2;

	int par_array_size = 0;
	int obs_array_size = 0;
	
	// int obs_array[obs];
	fd_set readfds;
	int fin_sd;

	// For Observer Port
	struct sockaddr_in sad2;
	struct sockaddr_in cad2;

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port\n");
		exit(EXIT_FAILURE);
	}

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	memset((char *)&sad2,0,sizeof(sad2)); /* clear sockaddr structure */
	sad2.sin_family = AF_INET; /* set family to Internet */
	sad2.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	port2 = atoi(argv[2]);
	if (port > 0 || port2 > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
		sad2.sin_port = htons((u_short)port2);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket for Particpant*/
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	///////////////////////////////////////////////////////////////////
	//For second bind for observer

	/* Create a socket for Observer*/
	sd2 = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd2 < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd2, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd2, (struct sockaddr *)&sad2, sizeof(sad2)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd2, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	initClientSpace();

	int stuff = 0;
	int new_soc = 0;
	alen = sizeof(cad);
	int n = 0;
	uint8_t nlen;
	char message[1000];

	while(1) {
		FD_ZERO(&readfds);
		FD_SET(sd, &readfds);
		FD_SET(sd2, &readfds);
		fin_sd = sd2;

		for (int i = 0; i < MAX; i++){	
			if(par_array[i].sd > 0){
				FD_SET(par_array[i].sd, &readfds);
			}
			if(par_array[i].sd > fin_sd){
				fin_sd = par_array[i].sd;
			}
		}

		for (int i = 0; i < MAX; i++){	
			if(obs_array[i].sd > 0){
				FD_SET(obs_array[i].sd, &readfds);
			}
			if(obs_array[i].sd > fin_sd){
				fin_sd = obs_array[i].sd;
			}
		}

		pending = checkPending(par_array, obs_array);

		// Choosing one of the select
		if (pending) {
			time.tv_sec = minTime.tv_sec;
			prevTime.tv_sec = time.tv_sec;
			stuff = select(fin_sd + 1, &readfds, NULL, NULL, &time);
		}
		else
			stuff = select(fin_sd + 1, &readfds, NULL, NULL, NULL);	
		

		if(stuff > 0){
			if(FD_ISSET(sd, &readfds)){
				if((new_soc = accept(sd, (struct sockaddr *)&cad, &alen)) < 0){
					fprintf(stderr, "Error: Accept failed\n");
					exit(EXIT_FAILURE);
				}

				// Check for an available spot for a new participant
				if (par_array_size < MAX) {
					send(new_soc, "Y", 1, 0);

					for (int i = 0; i < MAX; i++) {
						if (par_array[i].sd == -1) {
							par_array[i].sd = new_soc;
							par_array[i].coolDown.tv_sec = timeInSec;
							par_array_size++;
							break;
						}
					}
				}
				else {
					// No available slot for another participant
					send(new_soc, "N", 1, 0);
					close(new_soc);
				}

			}			
			// Observer
			else if(FD_ISSET(sd2, &readfds)) {

				if((new_soc = accept(sd2, (struct sockaddr *)&cad, &alen)) < 0){
					fprintf(stderr, "Error: Accept failed\n");
					exit(EXIT_FAILURE);
				}

				// Check for an available spot for a new observer
				if (obs_array_size < MAX) {
					//First send to the observer
					send(new_soc, "Y", 1, 0);

					for (int i = 0; i < MAX; i++) {
						if (obs_array[i].sd == -1) {
							obs_array[i].sd = new_soc;
							obs_array[i].coolDown.tv_sec = timeInSec;
							obs_array_size++;
							break;
						}
					}
				}
				else {
					// No available slot for another observer
					send(new_soc, "N", 1, 0);
					close(new_soc);
				}
			}


			for (int i = 0; i < MAX; i++) {

				// Observer **********************************************************************
				if (FD_ISSET(obs_array[i].sd, &readfds)) {
					if (strcmp(obs_array[i].username, " ") == 0) {
						n = recv(obs_array[i].sd, &nlen, sizeof(uint8_t), 0);

						if (n == 0) {
							close(obs_array[i].sd);
							obs_array[i].sd = -1;
							break;
						}

						n = recv(obs_array[i].sd, buf, nlen, 0);
						buf[nlen] = '\0';
						char retry = 'N';

						if (nlen <= 10) {
							// Checking the criteria for a valid username
							bool checkValid = true;
							int nameLen = strlen(buf);

							// Checking if the particpant has the name 
							for (int j = 0; j < MAX; j++) {
								if (strcmp(buf, par_array[j].username) == 0) {
									if (par_array[j].obs_sd == -1) {
										checkValid = true;
									}
									else {
										printf("Aleady in used by %s\n", par_array[j].username);
										checkValid = false;
										send(obs_array[i].sd, "T", 1, 0);
										retry = 'T';
									}
									break;
								}
								else {
									checkValid = false;
								}
							}

							if (retry == 'T') {
								break;
							}

							if (checkValid) {
								checkValid = validateName(buf, nameLen);

								if (checkValid) {
									strcpy(obs_array[i].username, buf);
									obs_array[i].coolDown.tv_sec = -1;
									minTime.tv_sec = 0;
									send(obs_array[i].sd, "Y", 1, 0);
									fprintf(stderr, "A new observer has joined\n");

									nlen = 25;
									for (int j = 0; j < MAX; j++) {
										if (obs_array[j].sd != -1){
											send(obs_array[j].sd, &nlen, sizeof(uint8_t), 0);
											send(obs_array[j].sd, "A new observer has joined", nlen, 0);

											if (strcmp(obs_array[i].username, par_array[j].username) == 0) {
												par_array[j].obs_sd = i;
												obs_array[i].par_sd = j;
											}
										}
									}
								}				
							}
							else {
								send(obs_array[i].sd, "N", 1, 0);
								close(obs_array[i].sd);
								obs_array[i].sd = -1;
								obs_array_size--;
							}
						}
						else {
							fprintf(stderr, "%s: is not valid\n", buf);
							send(obs_array[i].sd, "I", 1, 0);		// Invalid for enter more than 10 characters
						}
					}
					else if (recv(obs_array[i].sd, &nlen, sizeof(uint8_t), 0) == 0){	// Observer Ctrl+C
						int index = obs_array[i].par_sd;
						obs_array[i].sd = -1;
						obs_array[i].par_sd = -1;
						strcpy(obs_array[i].username," ");
						obs_array_size--;
						fprintf(stderr, "An observer has Disconnected\n");
						par_array[index].obs_sd = -1;
					}
				}

				// Participant **********************************************************************
				if (FD_ISSET(par_array[i].sd, &readfds)) {
					if (strcmp(par_array[i].username, " ") == 0) {
						// Setting up username for the participant
						n = recv(par_array[i].sd, &nlen, sizeof(uint8_t), 0);
						
						if (n == 0) {
							close(par_array[i].sd);
							par_array[i].sd = -1;
							break;

						}

						n = recv(par_array[i].sd, buf, nlen, 0);

						buf[nlen] = '\0';

						if (nlen <= 10) {
							// Checking the criteria for a valid username
							bool checkValid = true;
							int nameLen = strlen(buf);

							for (int j = 0; j < MAX; j++) {
								if (strcmp(buf, par_array[j].username) == 0) {
									checkValid = false;
									par_array[i].coolDown.tv_sec = timeInSec;
									send(par_array[i].sd, "T", 1, 0);
								}
							}

							if (checkValid) {
								checkValid = validateName(buf, nameLen);

								if (checkValid) {		///*********************************
									strcpy(par_array[i].username, buf);
									par_array[i].coolDown.tv_sec = -1;
									minTime.tv_sec = 0;

									send(par_array[i].sd, "Y", 1, 0);
									fprintf(stderr, "User %s has joined\n", par_array[i].username);
								}
								else {
									fprintf(stderr, "%s: is not valid\n", buf);
									send(par_array[i].sd, "I", 1, 0);	
								}						
							}
						}
						else {
							fprintf(stderr, "%s: is not valid\n", buf);
							send(par_array[i].sd, "I", 1, 0);		// Invalid
						}

					}
					else if (recv(par_array[i].sd, &nlen, sizeof(uint8_t), 0) == 0) {
						
						char new_message[1015];
						sprintf(new_message, "User %s has left", par_array[i].username);
						nlen = strlen(new_message);
						printAll(new_message, nlen);

						// D/C the observer
						int index = par_array[i].obs_sd;
						if (index != -1) {
							close(obs_array[index].sd);
							obs_array[index].sd = -1;
							obs_array[index].par_sd = -1;
							strcpy(obs_array[index].username," ");
							obs_array_size--;
						}

						// Disconnect participant
						DCPar(par_array, i, par_array_size);
					}
					else {
						// User send message
						n = recv(par_array[i].sd, message, nlen, 0);
						message[nlen] = '\0';

						// Checking for a private message
						if (checkMessage(message)) {
							char reciever[11];
							int count = 0;
							char new_message[1000];
							char final_message[1015];

							// get reciever name
							for (int j = 1; j < nlen; j++) {
								if (message[j] == ' ') {
									break;
								}
								else {
									reciever[j-1] = message[j];
									count++;
								}	
							}
							reciever[count] = '\0';
							int k = 0;

							// get the message inside
							for (int j = count+2; j < nlen; j++) {
								new_message[k] = message[j];
								k++;
							}

							new_message[k] = '\0';
							sprintf(final_message, "*%11s: %s", par_array[i].username, new_message);

							nlen = strlen(final_message);

							bool found = false;

							for (int j = 0; j < MAX; j++) {
								if (strcmp(reciever, par_array[j].username) == 0) {
									found = true;
									
									if (obs_array[par_array[i].obs_sd].sd != -1) {
										send(obs_array[par_array[i].obs_sd].sd, &nlen, sizeof(uint8_t), 0);
										send(obs_array[par_array[i].obs_sd].sd, final_message, nlen, 0);
									}

									if (obs_array[par_array[j].obs_sd].sd != -1) {
										send(obs_array[par_array[j].obs_sd].sd, &nlen, sizeof(uint8_t), 0);
										send(obs_array[par_array[j].obs_sd].sd, final_message, nlen, 0);
									}
									break;
								}
							}

							if (!found) {
								if (obs_array[par_array[i].obs_sd].sd != -1) {
									sprintf(final_message, "Warning: User %s doesn't exist...", reciever);
									nlen = strlen(final_message);
									send(obs_array[par_array[i].obs_sd].sd, &nlen, sizeof(uint8_t), 0);
									send(obs_array[par_array[i].obs_sd].sd, final_message, nlen, 0);
								}
							}

						}
						else {
							char new_message[1015];
							sprintf(new_message, ">%11s: %s", par_array[i].username, message);
							nlen = strlen(new_message);
							printAll(new_message, nlen);
							fprintf(stderr, ">%11s: %s\n", par_array[i].username, message);
						}
					}
				}
			}
		}
		else {
			//Time is up
			if (pending) {
				timeup(par_array, obs_array);
				minTime.tv_sec = 0;
				pending = false; 
			}
		}
	}	// end while

}


/**
*	Checking for any participant / observer who has not yet to enter a name
*	return - true (The Select will run with a timer)
*/
bool checkPending (par_sock par_array[], obs_sock obs_array[]) {
	pending = false;
	// Checking who has timer
	for (int i = 0; i < MAX; i++) {
		// Participant
		if (par_array[i].sd != -1 && par_array[i].coolDown.tv_sec > 0) {

			pending = true;
			if (minTime.tv_sec <= 0) {
				minTime.tv_sec = par_array[i].coolDown.tv_sec;
			}
			else if (par_array[i].coolDown.tv_sec < minTime.tv_sec) {
				minTime.tv_sec = par_array[i].coolDown.tv_sec;
			}
		}

		// Observer
		if (obs_array[i].sd != -1 && obs_array[i].coolDown.tv_sec > 0) {

			pending = true;
			if (minTime.tv_sec <= 0) {
				minTime.tv_sec = obs_array[i].coolDown.tv_sec;
			}
			else if (obs_array[i].coolDown.tv_sec < minTime.tv_sec) {
				minTime.tv_sec = obs_array[i].coolDown.tv_sec;
			}
		}
	}

	if (!pending)
		minTime.tv_sec = 0;
	return pending;
}


/** 
*	When time is up, go and update all the participant and observer timer
*/
void timeup (par_sock par_array[], obs_sock obs_array[]) {
	for (int i = 0; i < MAX; i++) {
		// Participant
		if (par_array[i].sd != -1 && par_array[i].coolDown.tv_sec > 0) {

			par_array[i].coolDown.tv_sec -= (prevTime.tv_sec - time.tv_sec);
			printf("par : %ld\n", par_array[i].coolDown.tv_sec);

			if (par_array[i].coolDown.tv_sec <= 0) {
				close(par_array[i].sd);
				par_array[i].coolDown.tv_sec = timeInSec;
				par_array[i].sd = -1;
			}
		}
		else {
			par_array[i].coolDown.tv_sec = -1;
		}

		// Observer
		if (obs_array[i].sd != -1 && obs_array[i].coolDown.tv_sec > 0) {

			obs_array[i].coolDown.tv_sec -= (prevTime.tv_sec - time.tv_sec);

			if (obs_array[i].coolDown.tv_sec <= 0) {
				close(obs_array[i].sd);
				obs_array[i].coolDown.tv_sec = timeInSec;
				obs_array[i].sd = -1;
			}
		}
		else {
			obs_array[i].coolDown.tv_sec = -1;
		}
	}
}


/**
*	Sending a message to all the valid observer(s)
*/ 
void printAll(char* message, int nlen) {
	for (int j = 0; j < MAX; j++) {
		if (obs_array[j].sd != -1){
			send(obs_array[j].sd, &nlen, sizeof(uint8_t), MSG_DONTWAIT);
			send(obs_array[j].sd, message, nlen, MSG_NOSIGNAL);
		}
	}
}


/** 
*	Take a peek at first character of the message to see if the message is for private
*/
bool checkMessage(char* message) {
	bool pri = false;

	if (message[0] == '@') {
		pri = true;
	}
	return pri;
}


/** 
*	Inintialize array space for both the participants and the observers
*/
void initClientSpace() {
	for (int i = 0; i < MAX; i++) {
		par_array[i].sd = -1;
		par_array[i].obs_sd = -1;
		strcpy(par_array[i].username," ");
		par_array[i].coolDown.tv_sec = 0;
		par_array[i].coolDown.tv_usec = 0;
	}

	for(int i = 0; i < MAX; i++){
		obs_array[i].sd = -1;
		obs_array[i].par_sd = -1;
		strcpy(obs_array[i].username, " ");
		obs_array[i].coolDown.tv_sec = 0;
		obs_array[i].coolDown.tv_usec = 0;
	}
}

/** 
*	Disconnecting a partipant and reset the par_array slot
*/
void DCPar(par_sock par_array[], int i, int par_array_size) {
	fprintf(stderr, "User %s has left\n", par_array[i].username);
	par_array[i].sd = -1;
	par_array[i].obs_sd = -1;
	strcpy(par_array[i].username," ");
	par_array_size--;
}


/** 
*	Validate a name that should follow the naming protocol
*/
bool validateName(char* buf, int nLen) {
	bool valid = true;
	for (int j = 0; j < nLen; j++) {
		if ((buf[j] >= 0 && buf[j] <= 47) || (buf[j] >= 58 && buf[j] <= 64) || (buf[j] >= 91 && buf[j] <= 96) || (buf[j] >= 123 && buf[j] <= 127)) {
			valid = false;
			break;
		}
	}
	return valid;
}