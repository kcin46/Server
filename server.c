#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "common.h"

int cookieGen(char ip[]){
	char temp[4];
	int temp_count=0, count=0, cookie=0;

	while(ip[count] != '\0'){
		while(ip[count] != '.' && ip[count] != '\0'){
			temp[temp_count++] = ip[count++];
		}
		temp[temp_count] = '\0';
		temp_count = 0;
		cookie = cookie + atoi(temp);
		memset(temp, 0, sizeof(temp));
		if(ip[count] != '\0'){
			count++;
		}
	}
	cookie = cookie * 13;
	cookie = cookie % 1111;
	return cookie;
}

int main(int argc, char *argv[]){
	
	//Makes sure there are enough arguments
	if(argc != 2){
		fprintf(stderr, "Incorrect number of arguments");
		exit(1);
	}
	in_port_t serverPort = atoi(argv[1]); //Save Port# from argument

	//Create socket for incoming connections
	/*Code from TCP/IP Sockets in C by Michael Donahoo and Kenneth Calvert*/
	int serverSock;
	if((serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		fprintf(stderr, "Creating the socket failed\n");
		exit(1);
	}
	
	//Constructs the local address structure
	/*Code from TCP/IP Sockets in C by Michael Donahoo and Kenneth Calvert*/
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(serverPort);

	 /*Code from TCP/IP Sockets in C by Michael Donahoo and Kenneth Calvert*/
	if(bind(serverSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
		fprintf(stderr, "Bind failed\n");
		exit(1);
	}

	 /*Code from TCP/IP Sockets in C by Michael Donahoo and Kenneth Calvert*/
	if(listen(serverSock, MAXPENDING) < 0){
		fprintf(stderr, "Listen failed\n");
		exit(1);
	}

	while(1){ //Run forever
		struct sockaddr_in clientAddr;
		socklen_t clientAddrLen = sizeof(clientAddr);

		//Wait for a client to connect
		 /*Code from TCP/IP Sockets in C by Michael Donahoo and Kenneth Calvert*/
			int clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &clientAddrLen);
			if(clientSock < 0){
				fprintf(stderr, "**Error** Accept function failed\n");
				continue; //drops this client and goes to the next iteration
			}

		 /*Code from TCP/IP Sockets in C by Michael Donahoo and Kenneth Calvert*/
		char clntName[INET_ADDRSTRLEN]; //String to contain client address
		if (inet_ntop(AF_INET, &clientAddr.sin_addr.s_addr, clntName, sizeof(clntName)) == NULL){
			fprintf(stderr, "**Error** Could not convert client address");
			continue;
		}
		
		/*Creates ipPort that has the format of "a.b.c.d:portNumber"*/
		char ipPort[25], portStr[10];
		strcpy(ipPort, clntName);
		sprintf(portStr, ":%s", argv[1]);
		strcat(ipPort, portStr);

		char readBuff[MAX_STR_SIZE+1];
		memset(readBuff, 0, MAX_STR_SIZE+1);
		int readResult;

		readResult = read(clientSock, readBuff, MAX_STR_SIZE+1);
		if(readResult < 0){
			fprintf(stdout, "**Error** Read Fail\n");
			fflush(stdout);
			close(clientSock);
			continue;
		}

		/*Checks if the buffer sent from the client is greater than the MAX_STR_SIZE*/ 
		if(readResult > MAX_STR_SIZE){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}

			/****************************************************************************************/
			/*At this point, the server is connected to a client with the clientSock file descriptor*/
			/****************************************************************************************/
		
		char helloStrs[4][MAX_STR_SIZE], temp[MAX_STR_SIZE];
		/*
		helloStrs[0] is magic string, helloStrs[1] is message type(HELLO), helloStrs[2] is login ID, 
		helloStrs[3] is name
		*/
		int count=0, temp_count=0, hello_count=0, argError = 0;
		
		while(readBuff[count] != '\0'){
			
			while(readBuff[count] != '\n' && readBuff[count] != ' ' && readBuff[count] != '\0'){
				
				/*If there are too many arguments, set argError to 1 and break from while loop*/
				if(hello_count >=4){
					argError = 1;
					break;
				}else{
					temp[temp_count++] = readBuff[count++];
				}
			} 
			/*If too many arguments, break from outer while loop*/
			if(argError == 1){
				break;
			}
			temp[temp_count] = '\0';
			temp_count = 0;
			strncpy(helloStrs[hello_count++], temp, sizeof(temp));
			memset(temp, 0, sizeof(temp));

			if(readBuff[count] != '\0'){
				count++;
			}

		} //end outer while

		/* Checks if there are too many arguments*/
		if(argError == 1){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}
		/*Checks if there are not enough arguments*/
		if(hello_count <4){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}
		
		/*Checks to make sure the magic string is correct*/
		if(strcmp(helloStrs[0], MAGIC_STRING) != 0){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}
	
		/*Checks if the message type is HELLO*/
		if(strcmp(helloStrs[1], "HELLO") != 0){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}
		
		/******************************************************************************/
		/*AT THIS POINT, THE HELLO MESSAGE WAS CORRECTLY SENT TO THE SERVER*/
		/******************************************************************************/	
	
		/*Formatting the string to send*/
		char statusStr[255], cookStr[5];
		int cookie = cookieGen(clntName);
		strcpy(statusStr, MAGIC_STRING);
		strcat(statusStr, " STATUS ");
		sprintf(cookStr, "%d", cookie);
		strcat(statusStr, cookStr);
		strcat(statusStr, " ");
		strcat(statusStr, ipPort);
		
		/*Send STATUS response to the client*/
		if(write(clientSock, statusStr, sizeof(char)*strlen(statusStr)) < 0){
			fprintf(stderr, "**Error** Write failed\n");
			continue;
		}
		
		/*Read from the socket for the last time with this client*/
		readResult = 0;
		memset(readBuff, 0, MAX_STR_SIZE + 1);
		readResult = read(clientSock, readBuff, MAX_STR_SIZE+1);
		if(readResult < 0){
			fprintf(stderr, "**Error** Read Fail\n");
			fflush(stderr);
			close(clientSock);
			continue;
		}

		/*Checks if the buffer sent from the client is greater than the MAX_STR_SIZE*/ 
		if(readResult > MAX_STR_SIZE){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}

		/*Reset variables*/
		count=0;
		temp_count=0;
		int clientBye_count=0;
		argError = 0;
		memset(temp, 0, sizeof(temp));
		char clientByeStrs[3][255];
		
		while(readBuff[count] != '\0'){
			
			while(readBuff[count] != '\n' && readBuff[count] != ' ' && readBuff[count] != '\0'){
				
				/*If there are too many arguments, set argError to 1 and break from while loop*/
				if(clientBye_count >=3){
					argError = 1;
					break;
				}else{
					temp[temp_count++] = readBuff[count++];
				}
			} 
			/*If too many arguments, break from outer while loop*/
			if(argError == 1){
				break;
			}
			temp[temp_count] = '\0';
			temp_count = 0;
			strncpy(clientByeStrs[clientBye_count++], temp, sizeof(temp));
			memset(temp, 0, sizeof(temp));

			if(readBuff[count] != '\0'){
				count++;
			}

		} //end outer while

		/* Checks if there are too many arguments*/
		if(argError == 1){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}
		/*Checks if there are not enough arguments*/
		if(clientBye_count < 3){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}
		
		/*Checks to make sure the magic string is correct*/
		if(strcmp(clientByeStrs[0], MAGIC_STRING) != 0){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}
	
		/*Checks if the message type is CLIENT_BYE*/
		if(strcmp(clientByeStrs[1], "CLIENT_BYE") != 0){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}
		
		/*Checks if the cookie sent from the client is the same cookie*/
		if(strcmp(clientByeStrs[2], cookStr) != 0){
			fprintf(stdout, "**Error** from %s\n", ipPort);
			fflush(stdout);
			close(clientSock);
			continue;
		}

		/**********************************************************************/
		/*AT THIS POINT, THE CLIENT_BYE MESSAGE HAS BEEN SENT PROPERLY*/
		/**********************************************************************/


		/*Setting up SERVER_BYE string and trying to write it to the socket*/
		memset(statusStr, 0, MAX_STR_SIZE);
		strcpy(statusStr, MAGIC_STRING);
		strcat(statusStr, " SERVER_BYE");
		if(write(clientSock, statusStr, sizeof(char)*strlen(statusStr)) < 0){
			fprintf(stderr, "**Error** SERVER_BYE write failed\n");
			fflush(stderr);
			continue;
		}else{
			/* If SERVER_BYE write happened then print out this information*/
			memset(statusStr, 0, MAX_STR_SIZE);
			strcpy(statusStr, cookStr);
			strcat(statusStr, " ");
			strcat(statusStr, helloStrs[2]);
			strcat(statusStr, " ");
			strcat(statusStr, helloStrs[3]);
			strcat(statusStr, " from ");
			strcat(statusStr, ipPort);
			fprintf(stdout, "%s\n", statusStr);
			fflush(stdout);
		}
		
		close(clientSock);	
	}

	return 0;	
}
