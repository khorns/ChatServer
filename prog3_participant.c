/*
*	prog3_participant.c
*	CSCI 367
*  
*	Description: Participant will be prompted for username input. Once approved, the client can
*					rapidly send message to the server as a public message or private.
*  
*	Sereyvathanak Khorn
* 	
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

int main( int argc, char **argv) {
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	int n; /* number of characters read */
	char buf[1000]; /* buffer for data from the server */

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

	char* username = (char *) malloc(100);
	uint8_t nlen;
	bool checkName = false;

	char buf2[2];
	char message[1001];

	n = recv(sd, buf, sizeof(buf), 0);
	bool conti = true;


	if (buf[0] == 'N') {
		printf("Connection Close!!!\n");
		close(sd);
	}
	else if (buf[0] == 'Y') {
		while (!checkName) {
			printf("Please enter a username: ");
			fgets(username, 100, stdin);

			nlen = strlen(username);
			nlen -= 1;

			n = send(sd, &nlen, sizeof(uint8_t), 0);
			n = send(sd, username, nlen, 0);

			n = recv(sd, buf2, 1, 0);
			buf2[1] = '\0';

			if (buf2[0] == 'Y') {

				checkName = true;
			}
			else if (buf2[0] == 'I' || buf2[0] == 'T') {
				// Repeat asking for a username
				checkName = false;
			}
		}

		while (conti) {
			fprintf(stderr, "Enter Message: ");
			fgets(message, 1000, stdin);

			nlen = strlen(message);
			nlen -= 1;
			message[nlen] = '\0';

			if (nlen > 0) {
				if (n = send(sd, &nlen, sizeof(uint8_t), 0) > 0) {
					send(sd, message, nlen, 0);					
				}
				else {
					close(sd);
					exit(0);
				}
			}
		}
	}

	close(sd);
	exit(EXIT_SUCCESS);
}
