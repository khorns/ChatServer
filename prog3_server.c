/*
*	prog3_server.c
*	CSCI 367
*  
*	Description: 
*  
*	Sereyvathanak Khorn
* 	
*/


/* demo_server.c - code for example server program that uses TCP */

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

/*------------------------------------------------------------------------
* Program: demo_server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection from a client
* (2) send a short message to the client
* (3) close the connection
* (4) go back to step (1)
*
* Syntax: ./demo_server port
*
* port - protocol port number to use
*
*------------------------------------------------------------------------
*/

typedef struct {
	int sd;				// its socket
	int obs_sd;			// Obs socket
	char username[10];  // Username
} par_sock;

struct timeval time = {60,0};

char move[1000];

par_sock par_array[par];


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
	// int port2; // Observer port
	uint16_t port;
	uint16_t port2;

	int par_array_size = 0;
	int obs_array_size = 0;

	
	int obs_array[obs];
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
	//For second bind

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

	// printf("Hello World\n");


	// Init the par_array
	for (int i = 0; i < MAX; i++) {
		par_array[i].sd = -1;
		par_array[i].obs_sd = -1;
		strcpy(par_array[i].username," ");
	}

	for(int j = 0; j < MAX; j++){
		obs_array[j] = -1;
	}

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
			if(obs_array[i] > 0){
				FD_SET(obs_array[i], &readfds);
			}
			if(obs_array[i] > fin_sd){
				fin_sd = obs_array[i];
			}
		}

		stuff = select(fin_sd + 1, &readfds, NULL, NULL, NULL);
		
		if(stuff > 0){
			if(FD_ISSET(sd, &readfds)){
				if((new_soc = accept(sd, (struct sockaddr *)&cad, &alen)) < 0){
					fprintf(stderr, "Error: Accept failed\n");
					exit(EXIT_FAILURE);
				}

				// Check for an available spot for a new participant
				if (par_array_size < MAX) {
					//First send to the participant
					send(new_soc, "Y", 1, 0);

					for (int i = 0; i < MAX; i++) {
						if (par_array[i].sd == -1) {
							par_array[i].sd = new_soc;
							par_array_size++;

							for (int j = 0; j < 5; j++) {
								printf("%d\n", par_array[j].sd);
							}
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

				// Check for an available spot for a new participant
				if (obs_array_size < MAX) {
					//First send to the participant
					send(new_soc, "Y", 1, 0);
					fprintf(stderr, "Connect an Observer\n");

					for (int i = 0; i < MAX; i++) {
						if (obs_array[i] == -1) {
							obs_array[i] = new_soc;
							obs_array_size++;

							for (int j = 0; j < 10; j++) {
								printf("%d\n", obs_array[j]);
							}
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


			for (int i = 0; i < MAX; i++) {
				if (FD_ISSET(par_array[i].sd, &readfds)) {
					if (strcmp(par_array[i].username, " ") == 0) {				// Continue with different messages
						// Setting up username for the participant
						// uint8_t nlen;
						n = recv(par_array[i].sd, &nlen, sizeof(uint8_t), 0);
						n = recv(par_array[i].sd, buf, nlen, 0);

						buf[nlen] = '\0';

						if (nlen <= 10) {
							// Checking the criteria for a valid username
							fprintf(stderr, "%s: is entered\n", buf);
							bool checkValid = true;
							int nameLen = strlen(buf);

							for (int j = 0; j < MAX; j++) {
								if (strcmp(buf, par_array[j].username) == 0) {
									printf("There is a duplicate!\n");
									checkValid = false;
									send(par_array[i].sd, "T", 1, 0);
								}
							}

							if (checkValid) {
								for (int j = 0; j < nameLen; j++) {
									if (buf[j] >= 0 && buf[j] <= 47) {
										checkValid = false;
										break;
									}

									if (buf[j] >= 58 && buf[j] <= 64) {
										checkValid = false;
										break;
									}

									if (buf[j] >= 91 && buf[j] <= 96) {
										checkValid = false;
										break;
									}

									if (buf[j] >= 123 && buf[j] <= 127) {
										checkValid = false;
										break;
									}

								}

								if (checkValid) {
									strcpy(par_array[i].username, buf);
									send(par_array[i].sd, "Y", 1, 0);
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
					// else if ((recv(par_array[i].sd, buf, sizeof(buf), 0)) == 0){		// Participant Disconnected
					else if (recv(par_array[i].sd, &nlen, sizeof(uint8_t), 0) == 0) {
						// Disconnect participant
						fprintf(stderr, "%s has left\n", par_array[i].username);
						par_array[i].sd = -1;
						par_array[i].obs_sd = -1;
						strcpy(par_array[i].username," ");
						par_array_size--;

					}
					else {
						// User send message
						n = recv(par_array[i].sd, message, nlen, 0);
						message[nlen] = '\0';

						fprintf(stderr, ">   %s: %s\n", par_array[i].username, message);

					}
					
				}
				
				else if (FD_ISSET(obs_array[i], &readfds)) {					// Observer Disconnected
					if ((n = recv(obs_array[i], buf, sizeof(buf), 0)) == 0){
						obs_array[i] = -1;
						obs_array_size--;
						fprintf(stderr, "An observer has Disconnected\n");
					}
				}
				
			}

		}


	}
	// end while

}