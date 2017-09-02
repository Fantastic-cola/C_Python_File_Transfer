/* 
 * Author:		Bolan Peng
 * Project 2: 	ftserver.c
 * Date:		8/6/2017
 * References: 	http://beej.us/guide/bgnet/
 * 				http://www.linuxhowtos.org/C_C++/socket.htm
 * 				CS344 - Lecture Notes & Project 4	
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>

#define IP_PROTOCOL 0
#define MAX_COMMAND_SIZE 10
#define MAX_FILENAME_SIZE 50
#define MAX_MESSAGE_SIZE 1024

void getCommand(int client_socketfd, char *command);
void processCommandG(int client_socketfd, char *filename, char *clientname, int port_num);
void processCommandL(int client_socketfd, char *clientname);
int getPortno(int client_socketfd);
void sendMessage(int client_socketfd, char *message);
int establishConnection(int portno);


int main(int argc, char *argv[]) {
	
	// Check usage
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Convert the port number from char to int
	int port_num = atoi(argv[1]);
	
	// Verify port number is valid
	if (port_num < 1024 || port_num > 65535) {
		perror("Error: Invalid port number!");
		exit(EXIT_FAILURE);
	}
	
	// Create network endpoints with socket
	int socketfd;
	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error: Cannot open socket!");
		exit(EXIT_FAILURE);
	}
	
	// Setting up an address
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port_num);
	server.sin_addr.s_addr = INADDR_ANY;
	
	// Bind socket to a port
	if (bind(socketfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("Error: Cannot bind socket!");
		exit(EXIT_FAILURE);
	}

	// Start listening for connections, max connection is 5
	if (listen(socketfd, 5) < 0) {
		perror("Erro: Error listening for connections!");
		exit(EXIT_FAILURE);
	}
	printf("Server open on %d\n", port_num);
	
	// Define some variables necessary for the connections
	int client_socketfd, data_socketfd;
	struct sockaddr_in client_add;
	socklen_t client_len = sizeof(client_add);
	char command[MAX_COMMAND_SIZE];
	char filename[MAX_FILENAME_SIZE];
	int n, portno;
	char ack[] = "ok";
	
	// Loop and accept connections until SIGINT is received
	while (1) {
		// Accept connection
		client_socketfd = accept(socketfd, (struct sockaddr *) &client_add, &client_len);
		// Close bad connection
		if (client_socketfd < 0) {
			perror("Error: Accept call failed!");
			close(client_socketfd);
		}	
		
		/***************** Fork off to process the client *******************************/
		pid_t childPid = fork();
		
		// If fork failed
		if (childPid < 0) {
			printf("Error: Forking failed!\n");
			exit(EXIT_FAILURE);
		}
		
		// Child process
		else if (childPid == 0) {
			// Print client address
			char clientname[INET_ADDRSTRLEN];
			// REFERENCE: https://stackoverflow.com/questions/7483301/how-do-i-print-client-address-while-running-server-code-in-socket	
			printf("Connection from %s\n", inet_ntop(AF_INET, &client_add.sin_addr, clientname, sizeof(clientname)));
					
			getCommand(client_socketfd, command);
			
			// If command is to get a file		
			if (strcmp(command, "-g") == 0) {
				processCommandG(client_socketfd, filename, clientname, port_num);
				exit(EXIT_SUCCESS);
			}
			
			// If command is to list directory
			else if (strcmp(command, "-l") == 0) {
				processCommandL(client_socketfd, clientname);
				exit(EXIT_SUCCESS);
			}
			
			// If it is a bad command
			else {
				// ACK receving the command
				sendMessage(client_socketfd, ack);
				// Receive the data port number
				portno = getPortno(client_socketfd);
				// ACK receving the data port number
				sendMessage(client_socketfd, ack);
				// Send error message
				char message[] = "INVALID COMMAND";
				sendMessage(client_socketfd, message);
				exit(EXIT_FAILURE);
			}
		}
		
		// Parent process
		else {
			close(client_socketfd);
		}
	}
	return 0;
}

/*
 * Takes a socket file descriptor and a reference to a command
 * Receives a command from ftclient and stores it in the value of command variable
 */
void getCommand(int client_socketfd, char *command) {
	int n;
	
	// Receive command
	bzero(command, MAX_COMMAND_SIZE);	
	n = read(client_socketfd, command, MAX_COMMAND_SIZE);			
	if (n < 0) {
		perror("Error: Error reading from socket");
		exit(EXIT_FAILURE);
	} 
	else if (n == 0) {						
		printf("Connection closed by client; exiting\n");
		close(client_socketfd);
		exit(EXIT_SUCCESS);
	}
}

/*
 * Takes a socket file descriptor, reference to a filename and a client name
 * Receives filename, port number and establishes a data connection to the port number
 * And send the file with the specified filename over the data connection
 */
void processCommandG(int client_socketfd, char *filename, char *clientname, int port_num) {
	int n, portno;
	int data_socketfd;
	char ack[] = "ok";
	
	// ACK receving the command
	sendMessage(client_socketfd, ack);
	
	// Receive filename
	bzero(filename, MAX_FILENAME_SIZE);
	n = read(client_socketfd, filename, MAX_FILENAME_SIZE);		
	if (n < 0) {
		perror("Error: Error reading from socket");
		exit(EXIT_FAILURE);
	} 
	// ACK receving the filename
	sendMessage(client_socketfd, ack);
	// Receive the data port number
	portno = getPortno(client_socketfd);
	// ACK receving the data port number
	sendMessage(client_socketfd, ack);

	// Connect to data port
	data_socketfd = establishConnection(portno);
								
	printf("File \"%s\" requested on port %d\n", filename, portno);
				
	/*************Send file to client************************/
	FILE *fp;
	unsigned int file_length;
	fp = fopen(filename, "r");
				
	// If file is not found
	if (fp == NULL) {
		printf("File not found. Sending error message to %s:%d\n", clientname, port_num);
		// send "FILE NOT FOUND" to control connection
		char errorMsg[] = "FILE NOT FOUND";
		sendMessage(client_socketfd, errorMsg);
		exit(EXIT_FAILURE);
	}
				
	// Else, send message to client indicate this is going to a file transfer
	char message[] = "DATA";
	sendMessage(client_socketfd, message);
				
	// Process the file
	// Get the size of the file
	fseek(fp, 0, SEEK_END);
	file_length = ftell(fp);
	rewind(fp);
				
	// Create buffer to store the file data, up to the size of the text file
	char *buffer = malloc(file_length);
				
	// Read file into the buffer
	n = fread(buffer, sizeof(char), file_length, fp);
	if (n < 0) {
		perror("Error: Error reading the textfile!");
		exit(EXIT_FAILURE);
	}
	close(fp);
				
	// Send the file over the data socket
	// First send the size of the file
	n = write(data_socketfd, &file_length, sizeof(unsigned));
	if (n < 0) {
		perror("Error: Error writing file length to socket!");
		exit(EXIT_FAILURE);
	}
				
	// Then send the file itself
	printf("Sending \"%s\" to %s:%d\n", filename, clientname, portno);
	n = write(data_socketfd, buffer, file_length);
	if (n < 0) {
		perror("Error: Error writing text file to socket!");
		exit(EXIT_FAILURE);
	}
	close(data_socketfd);
}

/*
 * Takes a socket file descriptor
 * Receives port number, establishes a data connection to the port number
 * And send the list of files in the directory over the data connection
 */
void processCommandL(int client_socketfd, char *clientname) {
	int n, portno;
	int data_socketfd;
	char ack[] = "ok";
	
	// ACK receving the command
	sendMessage(client_socketfd, ack);
	// Receive the data port number
	portno = getPortno(client_socketfd);
	// ACK receving the data port number
	sendMessage(client_socketfd, ack);
	// Connect to data port
	data_socketfd = establishConnection(portno);
				
	printf("List directory requested on port %d\n", portno);
				
	// Get the list of files in current directory
	// REFERENCE: http://www.studytonight.com/c/program-to-list-files-in-directory.php
	char directory[1024] = "";
	struct dirent *dir;
	DIR *d = opendir(".");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			// store the file name in directory variable
			strncat(directory, dir->d_name, strlen(dir->d_name));
			strncat(directory, "\n", 2);
		}
		closedir(d);
	}
				
	// Send directory list to ftclient
	printf("Sending directory contents to %s:%d\n", clientname, portno);
	n = write(data_socketfd, directory, strlen(directory));
	if (n < 0) {
		perror("Error: Error writing to socket");
		exit(EXIT_FAILURE);
	}
	close(data_socketfd);	
}

/* 
 * Takes a socket file descriptor
 * Receives port number as a string from ftclient
 * Returns a port number as an int
 */
int getPortno(int client_socketfd) {
	int n;
	char port[10];
	
	n = read(client_socketfd, port, 10);
	if (n < 0) {
		perror("Error: Error reading from socket");
		exit(EXIT_FAILURE);
	} 
	return atoi(port);
}

/* 
 * Takes a socket file descriptor and a reference to a message
 * Sends the value of the message to ftclient
 */
void sendMessage(int client_socketfd, char *message) {
	int n;
	n = write(client_socketfd, message, strlen(message));
	if (n < 0) {
		perror("Error: Error writing to socket");
		exit(EXIT_FAILURE);
	}
}

/*
 * Takes a port number
 * Establishes a connection to ftclient's data port
 * Return a socket file descriptor
 */
int establishConnection(int portno) {
	int socketfd, err;
	struct sockaddr_in serv_addr;
	char hostname[1024];
	
	// REFERENCE: https://stackoverflow.com/questions/5190553/linux-c-get-server-hostname
	gethostname(hostname, 1024);
	struct hostent *server = gethostbyname(hostname);
	if (server == NULL) {
		perror("Error: no such host");
		exit(EXIT_FAILURE);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));		// Initialize serv_addr to zero
    serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);	// Copies the characters from server to serv_addr 
    serv_addr.sin_port = htons(portno);			// Converts port number to network byte order
	
	// Open the socket
	socketfd = socket(AF_INET, SOCK_STREAM, IP_PROTOCOL);
	if (socketfd < 0) {
		perror("Error: Couldn't open socket!");
        exit(EXIT_FAILURE);
	}
	
	// Connect over the socket
    err = connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (err < 0) {
        perror("Error: Couldn't connect to the socket!");
        exit(EXIT_FAILURE);
    }
	return socketfd;
}
