/* Regena Villaroman & Josh Reavis
 * CS 367 Program 1 Server
 * Winter 2017
 */

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define	QLEN 6	/* size of request queue */

int main(int argc, char **argv)
{
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, sd2; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */
	char buf[1000]; /* buffer for string the server sends */
	char *word;
	pid_t childId;

	if (argc != 3){
		fprintf(stderr, "Error: Wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	if (port > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
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

	/* set up secret word */
	word = argv[2];
	for (int i = 0; i < strlen(word); i++){
		if ((word[i] < 97) || (word[i] > 122)){
			fprintf(stderr, "Error: secret word can only contain lowercase letters\n");
			exit(EXIT_FAILURE);
		}
	}

	signal(SIGCHLD, SIG_IGN);
	while (1){
		alen = sizeof(cad);
		if ((sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0){
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		} else {
			/* fork */
			childId = fork();
			if (childId < 0){						/* error */
				fprintf(stderr, "Error: fork failed\n");
			} else if (childId == 0){				/* child - play game */
				close(sd);

				int guesses = strlen(word);
				int charFound = 0;
				char board[guesses];
				int guessCorrect;
				char currGuess;
				for (int i = 0; i < guesses; i++){
					board[i] = '_';
				}

				while ((guesses > 0) && (charFound < strlen(word))){
					/* send number of guesses to client */
					send(sd2, &guesses, sizeof(guesses), MSG_NOSIGNAL);

					/* send board */
					sprintf(buf, "%s", board);
					send(sd2, buf, sizeof(buf), MSG_NOSIGNAL);
					
					/* get guess */
					memset(buf, '\0', sizeof(buf));
					recv(sd2, &currGuess, sizeof(char), MSG_WAITALL);
					guessCorrect = 0;
					for (int i = 0; i < strlen(word); i++){
						if ((word[i] == currGuess) && (board[i] == '_')){
							board[i] = currGuess;
							guessCorrect = 1;
							charFound++;
						}
					}
					if (!guessCorrect){
						guesses--;
					}
				}
				if (guesses == 0){				/* user lost */
					send(sd2, &guesses, sizeof(guesses), MSG_NOSIGNAL);
					send(sd2, board, strlen(board), MSG_NOSIGNAL);
				} else {						/* user won */
					guesses = 255;
					send(sd2, &guesses, sizeof(guesses), MSG_NOSIGNAL);
					send(sd2, board, strlen(board), MSG_NOSIGNAL);
				}
				close(sd2);
				exit(0);
			}
			/* else parent - go back to listening */
			close(sd2);
		}
	}
	close(sd);
}
