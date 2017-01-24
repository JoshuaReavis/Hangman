/* Regena Villaroman & Josh Reavis
 * CS 367 Program 1 Client
 * Winter 2017
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

int main(int argc, char ** argv)
{
	struct hostent *ptrh;			/* pointer to a host table entry */
	struct protoent *ptrp;			/* pointer to a protocal table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	int n; /* number of characters read */
	char buf[1000]; /* buffer for data from the server */
	int victory = 0;
	char letter;

	int guesses;

	memset((char *)&sad, 0, sizeof(sad));		/* clear sockaddr structure */
	sad.sin_family = AF_INET;					/* set family to Internet */

	if (argc != 3){
		fprintf(stderr, "Error: Wrong number of arguments.\nusage:\n./cliernt server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	/* port setup */
	port = atoi(argv[2]);
	if (port > 0){
		sad.sin_port = htons((u_short)port);
	} else {
		fprintf(stderr, "Error: bad port number %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	/* host setup */
	host = argv[1];
	ptrh = gethostbyname(host);
	if (ptrh == NULL){
		fprintf(stderr, "Error: invalid host: %s\n", host);
		exit (EXIT_FAILURE);
	}
	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Protocol setup */
	if (((long int)(ptrp = getprotobyname("tcp"))) == 0){
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0){
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect socket to server */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0){
		fprintf(stderr, "connect failed\n");
		exit(EXIT_FAILURE);
	}

	while(1) {
		/* get number of guesses */
		memset(buf, '\0', sizeof(buf));
		recv(sd, &guesses, sizeof(guesses), MSG_WAITALL);
		if (guesses == 255){
			memset(buf, '\0', sizeof(buf));
			recv(sd, buf, sizeof(buf), MSG_WAITALL);
			printf("Board: %s\nYou won!\n", buf);
			close(sd);
			exit(0);
		} else if (guesses == 0){
			memset(buf, '\0', sizeof(buf));
			recv(sd, buf, sizeof(buf), MSG_WAITALL);
			printf("Board: %s\nYou lost.\n", buf);
			close(sd);
			exit(0);
		}

		/* get board */
		memset(buf, '\0', sizeof(buf));
		recv(sd, buf, sizeof(buf), MSG_WAITALL);
		
		/* display guesses remaining and current board */
		printf("\nBoard: %s (%d guesses left)\n ", buf, guesses);
		fflush(stdout);

		/* get guess */
		printf("Enter guess: ");
		fflush(stdout);
		memset(buf, '\0', sizeof(buf));
		read(STDIN_FILENO, buf, sizeof(buf));
		letter = buf[0];
		send(sd, &letter, sizeof(char), MSG_NOSIGNAL);
		printf("\n");
	}
	
}
