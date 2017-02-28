/* demo_client.c - code for example client program that uses TCP */

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

/*------------------------------------------------------------------------
* Program: demo_client
*
* Purpose: allocate a socket, connect to a server, and print all output
*
* Syntax: ./demo_client server_address server_port
*
* server_address - name of a computer on which server is executing
* server_port    - protocol port number server is using
*
*------------------------------------------------------------------------
*/
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

	char* username = (char *) malloc(10);
	uint8_t nlen;
	bool checkName = false;

	char buf2[2];
	char message[1000];

	n = recv(sd, buf, sizeof(buf), 0);

	if (buf[0] == 'N') {
		printf("Connection Close!!!\n");
		close(sd);
	}
	else if (buf[0] == 'Y') {
		while (!checkName) {
			printf("Please enter a username:\n");
			scanf("%s", username);
			// fgets(username, 10, stdin);
			nlen = strlen(username);

			// fprintf(stderr, "username: %s\n", username);
			// fprintf(stderr, "nlen: %d\n", nlen);

			send(sd, &nlen, sizeof(uint8_t), 0);
			send(sd, username, nlen, 0);

			n = recv(sd, buf2, 1, 0);
			buf2[1] = '\0';
			// fprintf(stderr, "%s\n", buf2);

			if (buf2[0] == 'Y') {

				checkName = true;
			}
			else if (buf2[0] == 'I' || buf2[0] == 'T') {
				// Repeat asking for a username
				checkName = false;
			}
		}
		fgets(message, 1000, stdin);

		while (1) {

			// strcpy(message, "");
			// char* message = (char*) malloc(1000); 
			// memset(message, '\n', 1);
			// fprintf(stderr, "message: %s\n", message);
			// fprintf(stderr, "nlen: %d\n", nlen);
			fprintf(stderr, "Enter Message: ");
			fgets(message, 1000, stdin);

			nlen = strlen(message);
			nlen -= 1;
			message[nlen] = '\0';

			if (nlen > 0) {
				// fprintf(stderr, "%s\n", message);
				// fprintf(stderr, "%d\n", nlen);
				// scanf("%[^\n]%*c", message);

				send(sd, &nlen, sizeof(uint8_t), 0);
				send(sd, message, nlen, 0);
			}
		}
	}


	close(sd);

	exit(EXIT_SUCCESS);
}