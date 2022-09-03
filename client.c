
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define REQRES_SIZE 65536
static char traderID[100];	 // This memory holds the traders id
static char pass[100];		 // This char array holds the password of the user for entire login session
static struct sockaddr_in S;    // sockaddr_in is a predefined structure that holds the socket's information.
static int trader_number;	 // This variable holds the trader number used for the current session

// Function to send request to the server
void sendRequest(const char *request, char *response) 
{
	int sock = 0, t = 0, len_req = strlen(request), offset = 0;
	char *buffer[1024];
	
	//Initializing the socket.
	sock = socket(AF_INET, SOCK_STREAM, 0); 

	if (sock < 0)
	{
		printf("Client Error: Socket is not found!\nExiting.\n");
		exit(-1);
	}

	//Sending connection request to the server
	t = connect(sock, (struct sockaddr*)&S, sizeof(S)); 
	if (t < 0)
	{
		printf("Client Error: Failed to establish connection with the server\n");
		exit(-1);
	}
	
	//Sending the request to the server
	t = send(sock, request, len_req, 0); 
	if (t < 0)
	{
		printf("Client Error: Request not sent.\nPlease try again!\n");
		exit(-1);
	}

	while(1) 
	{
		int res = 0, len =  1024;
		
		res = recv(sock, buffer, len, 0);
		if (res > 0)
		{
			//Save response from the server to client into a buffer.
			char *temp_value = response + offset;
			memcpy(temp_value, buffer, res);
			offset = offset + res;
		}
		else 
		{
			break;
		}
	}

	response[offset] = '\0';
}

int checkStatus(char *response) //This fucntion checks if the sent request was succesfull or not
{
	char tmp[8];
	strncpy(tmp, response, 8);
	// response is succesfull if tmp contains the word ACCEPTED
	if (strcmp(tmp, "ACCEPTED") == 0)
	{
		return 1;
	}
	
	return 0;
}

int loginAuth(char *response) // This function is used for authentication of the user
{
	char request[REQRES_SIZE];
	memset(request, '\0', REQRES_SIZE);

	strcat(request, traderID);
	strcat(request, " ");
	strcat(request, pass);
	strcat(request, " ");
	strcat(request, "L");
	strcat(request, " $"); // these are the ending character markers for each request sent to mark the end of request
	sendRequest(request, response);

	if (checkStatus(response) > 0)
	{
		return 1;
	}
	
	return 0;
}

int buy_sell(int item_number, int quantity, int unit_price, char type)
{ // This function create the request to send over the server based on the activity the client wants to do
	char request[REQRES_SIZE], response[REQRES_SIZE], tmp[32];
	
	memset(request, '\0', REQRES_SIZE);
	memset(response, '\0', REQRES_SIZE);

	sprintf(tmp, "%d", trader_number);
	strcat(request, tmp);
	strcat(request, " ");
	strcat(request, pass);
	strcat(request, " ");

	switch(type)
	{
		case 'b':strcat(request, "B ");
			 break;
		case 's':strcat(request, "S ");
			 break;
	}
	

	sprintf(tmp, "%d", item_number);
	strcat(request, tmp);
	strcat(request, " "); // creation  of request by appending to the string named request using strcat function
	sprintf(tmp, "%d", quantity);
	strcat(request, tmp);
	strcat(request, " ");
	sprintf(tmp, "%d", unit_price);
	strcat(request, tmp);
	strcat(request, " $");

	sendRequest(request, response);

	if (checkStatus(response) > 0)
	{
		return 1;
	}
	printf("Request Failed.\nPlease try again!\n");
	return 0;
}

void view(char type) //This function generates a request to send to the server in order to see the user's trades or open orders.
{
	char response[REQRES_SIZE],request[REQRES_SIZE], tmp[32];
	
	memset(request, '\0', REQRES_SIZE);
	memset(tmp, '\0', REQRES_SIZE);
	
	sprintf(tmp, "%d", trader_number);
	strcat(request, tmp);
	strcat(request, " ");
	strcat(request, pass);
	strcat(request, " ");

	if (type == 'o') //This condition indicates that the request is a 'view order' request
	{
		strcat(request, "VO $");
	}
	else if (type != 'o') //This condition indicates that the request is a 'view trade' request
	{
		strcat(request, "VT $");
	}
	sendRequest(request, response);

	if (checkStatus(response) == 0)
	{
		printf("Request Failed.\nPlease try again!\n");
		return;
	}

	char *check = response;
	printf("%s", check);
}

void menu() // This function provides us all the operations that we can perform on our Trading system
{
	printf("\n");
	printf("******** Services offered ******** \n");
	printf("1. View Orders\n");
	printf("2. View Trades\n");
	printf("3. Buy Reuest\n");
	printf("4. Sell Request\n");
	printf("5. Correct trader number\n");
	printf("6. Exit\n");
	printf("Enter your choice: ");
}

int main(int argc, char const *argv[])
{
	int opt, item_number, quantity, unit_price;
	
	if(argc < 3){
		printf("-----> Usage: ./client <Server IP Address> <Server port number>\n");
		printf("Please try again!\n");
		return 0;
	}
	S.sin_addr.s_addr = inet_addr(argv[1]); //first command line argument as the server address
	S.sin_family = AF_INET;		 //IPv4 is used for the network
	S.sin_port = htons(atoi(argv[2]));	 //Second command line argument as the port number
	
	printf("\n");
	printf("------------------------------------Welcome------------------------------------\n");
	
	while (1)
	{
		char response[REQRES_SIZE];
		
		memset(response, '\0', REQRES_SIZE);
		printf("Please enter user ID: ");
		scanf("%s", traderID);
		printf("Please enter your password %s: ", traderID);
		scanf("%s", pass);

		int retVal = loginAuth(response);
		char* check = response;
		
		printf("%s", check);
		
		if (retVal)
		{
			char ch = 'y';
			while(ch == 'y')
			{
				printf("Enter your trader number as shown by the trading system\n");
				scanf("%d", &trader_number);
				printf("You have entered trade number: %d. Would you like to change or update the trade number?(y/n)? \n", trader_number);
				scanf("%c%c", &ch, &ch);
			}
						
			break;
		}
	}

	
	//Services offered by the trading system
	while(1) 
	{
		menu();
		scanf("%d", &opt);
		
		switch(opt)
		{
			case 1: view('o');
				break;
				
			case 2: view('t');
				break;
				
			case 3: printf("Enter the item ID of the item you want to buy: \n");
				scanf("%d", &item_number);
				if(item_number>9){
					printf("Wrong item number!\nSelect item number in range 0 to 9!\n");
					break;
				}
				printf("Quantity: \n");
				scanf("%d", &quantity);
				printf("Per Unit Price: \n");
				scanf("%d", &unit_price);
				buy_sell(item_number, quantity, unit_price, 'b');		
				break;
				
			case 4: printf("Enter the item ID of the item you want to sell: \n");
				scanf("%d", &item_number);
				if(item_number>9){
					printf("Wrong item number!\nSelect item number in range 0 to 9!\n");
					break;
				}
				printf("Quantity: \n");
				scanf("%d", &quantity);
				printf("Per Unit Price: \n");
				scanf("%d", &unit_price);
				buy_sell(item_number, quantity, unit_price, 's');
				break;

			case 5: printf("Enter your trader number as shown by the trading system\n");
				scanf("%d", &trader_number);
				break;
				
			case 6: exit(-1);
			
			default:printf("Invalid Choice. Try again\n");
		}
	}
	return 0;
}
