//including required library files
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "server.h"


int main(int argc, char const *argv[])
{
  	// Validating the arguments 
	if(argc != 2)
	{
		printf("-----> Usage: ./server <PORT number>\nPlease Try again!\n");
		return 0;
	}
	if ((atoi(argv[1]) < 1024))
	{
		printf("Invalid Port Number. Port number shoud have value > 1024\nPlease try again.\n");
		return 0;
	}

	printf("Server Initialised...\n\n");
  	// Creating a new communication endpoint and checking for any error
	int serverSockFD; // Server socket file descriptor
	if ((serverSockFD = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error: Socket Creation Failed\n");
		return 0;
	}

	struct sockaddr_in serverAddr;

	bzero((char *)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(atoi(argv[1]));

	// Attaching a local host address to our socket and checking for any error
	if (bind(serverSockFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
	{
		printf("Server Error: Socket Binding Failed\n");
		return 0;
	}
	
	
	// Listening to 5 simultaneous connections
	if (listen(serverSockFD, 5) == -1)
	{
		printf("Server Error: Socket Listening Failed\n");
		return 0;
	}
	
	struct sockaddr_in clientAddr;
  	int clientAddrStructSize = sizeof(clientAddr), tempSockFD;
  	char primaryBuff[65536];

  	while (1)
  	{
    		tempSockFD = accept(serverSockFD, (struct sockaddr *)&clientAddr, &clientAddrStructSize);
    		int pbofpos = 0;
		if (tempSockFD == -1)
		{
			printf("Server Error: accept() Failed\n");
			continue;
		}
    		// Request from client is first stored in secondaryBuff and appropriate action is taken further
 	 
  		int primaryOffset = 0;
    		while (1)
    		{
			char secondaryBuff[512];
			int length = 0, hasEnded = 0, secondaryOffset = 0;

			memset(secondaryBuff, '\0', sizeof(secondaryBuff));
      			secondaryOffset = read(tempSockFD, secondaryBuff, sizeof(secondaryBuff));

      			if (secondaryOffset == -1)
		        {
				printf("Server Error(): read() failed\n");
				exit(0);
		      	}

      			// After getting request from client value is copied to primaryBuff
      			memcpy(primaryBuff + primaryOffset, secondaryBuff, secondaryOffset);
      			primaryOffset = primaryOffset + secondaryOffset;

	 		//checking if delimiter found or not
     			int delimFound = 0, secBuffSize = 0;
     			char *delimiter = "$";
	  		
			    secBuffSize = sizeof(secondaryBuff);
		      	for (int pos = 0; pos < secBuffSize - 1; pos++)
		      	{
					if (secondaryBuff[pos] == '$')
					{
						delimFound++;
						break;
					}
		      	}
		      
		      	//delimiter found
		        if (delimFound != 0)
					break;
    		}

		//splitting the contents of primary buffer by spaces and storing it in array of strings
	    	primaryBuff[primaryOffset] = '\0';
	    	printf("Requested Query- \n%s", primaryBuff); 
	    	printf("\n");
	    
			int wordCount = 0, index = 0;
			for (int i = 0; i < strlen(primaryBuff); i++)
			{
					if (isspace(primaryBuff[i]))
						wordCount++;
			}
			char **requestmessage = (char **)malloc(wordCount * sizeof(char *));
			char *token = strtok(primaryBuff, " ");
  		
			while (token)
			{
					char *word = token;
					requestmessage[index] = (char *)malloc(strlen(word) * sizeof(char));
					strcpy(requestmessage[index], token);
					token = strtok(NULL, " ");
					index++;
			}


	    	char *action = requestmessage[2];
    
    		// Client requests for Login
    		if (strcmp(action, "L") == 0)
    		{
	      		char *ps = requestmessage[1];
	     		char *uname = requestmessage[0];

	      		// Checking the credentials using our above defined function
	      		struct authClientDetails ret = authCred(uname, ps, 0, 'n');
	      
	      		// username and password matched
	      		if (ret.retVal == 1)
	      		{ 
        			char num2sendstring[10];
        			int num2send = ret.number;
        			char message[] = "ACCEPTED - \n>>>Login Successfull for Trade ID: ";
        			sprintf(num2sendstring, "%d\n\n", num2send);
        			strcat(message, num2sendstring);
        			sendmessage(tempSockFD, message);
				}
      			else  // Either password not matched or username not in database
      			{ 
        			char *message = "REJECTED- \n>>>Incorrect Password\nPlease try again!\n\n";
        			sendmessage(tempSockFD, message);
				}
   			}
    		else if (strcmp(action, "S") == 0)  // Client requests for selling an item
    		{
    			//getting the password and trader id of the current trader
      			char *pass = requestmessage[1];
      			char *uid = requestmessage[0];
      
      			//getting authorization details for that trader
      			struct authClientDetails authcheck = authCred("", pass, uid, 'i');
      			char username[50];
      			strcpy(username, authcheck.name);
			
		
	  		//invalid user
			if (authcheck.retVal != 1)
      			{
				sendmessage(tempSockFD, "REJECTED \n\nLogin Unsuccessful. Invalid trader number\n\n");
        			close(tempSockFD);
        			continue;
      			}
				int userid = atoi(uid);
	      		struct tradeItem sell; //details of current request
	      		strcpy(sell.username, username);
      
      			//item no. of item for trade
				int item = atoi(requestmessage[3]);
				sell.itemnumber = item;
      
				//quantity of that item
				sell.qty = atoi(requestmessage[4]);;
		
				//unit price for selling
				sell.price = atoi(requestmessage[5]);
				sell.id = userid;
      
	      		sell.type = 'S';  //'S' is for selling
	  
				/*
					now we either sell that item if any matched buy request is found in buy queue 
					for that item or we put that item in the sell queue
				*/
				while (sell.qty > 0)
				{
				// When there are no requests in the buy queue, add current request to the sell queue
					if (firstLastIndexforBuy[item][0] == firstLastIndexforBuy[item][1])
					{
						queueinsert(sell);
						break;
					}
			
					//else we find the the matched request in the buy queue
					int bestsell = firstLastIndexforBuy[item][0];
			
					//scanning the buy queue and finding the highest paying buy request
					for (int i = firstLastIndexforBuy[item][0] + 1; i != firstLastIndexforBuy[item][1]; i = (i + 1) % 1000)
					{
						if ((buyQueue[i][item].price > buyQueue[bestsell][item].price))
							bestsell = i;
					}
        
        		//if highest price is less than the price at which the trader is willing to sell
        		//the request will be put in sell queue
					if (buyQueue[bestsell][item].price < sell.price)
					{
						queueinsert(sell);
						break;
					}
        
        		//matched trade found
        		else
       			{
          			if (buyQueue[bestsell][item].qty <= sell.qty)
          			{
          				//updating sell quantity
            				sell.qty -= buyQueue[bestsell][item].qty;
            
            				//creating trade for the matched item 
				    	//trade contains buyer's name, seller's name, item no. , quantity and price 
				    	struct clientDetails temp;
				    	temp.price = buyQueue[bestsell][item].price;
				    	temp.qty = buyQueue[bestsell][item].qty;

				    	strcpy(temp.seller, username);
				   		strcpy(temp.buyer, buyQueue[bestsell][item].username);

				    	temp.itemnumber = item;
				    	temp.buyerid = buyQueue[bestsell][item].id;
				    	temp.sellerid = sell.id;
				    
				    	//putting the trade in tradeDetails array
				    	tradeDetails[tradeCount++] = temp;
		
						//shifting the buy requests in the buy queue and updating the ending position
            			for (int i = bestsell; i != firstLastIndexforBuy[item][1]; i = (i + 1) % 1000)
              					buyQueue[i][item] = buyQueue[(i + 1) % 1000][item];
            				
						firstLastIndexforBuy[item][1] = (1000 + firstLastIndexforBuy[item][1] - 1) % 1000;
  					}
          			else
          			{
					    //quantity for buying is more than the quantity for sell
    					buyQueue[bestsell][item].qty -= sell.qty;
            
						//creating trade for that item
						struct clientDetails temp;
						temp.price = buyQueue[bestsell][item].price;
						temp.qty = sell.qty;

						strcpy(temp.seller, username);
						strcpy(temp.buyer, buyQueue[bestsell][item].username);

						temp.itemnumber = item;
						temp.buyerid = buyQueue[bestsell][item].id;
						temp.sellerid = sell.id;
		
						//inserting the trade into tradeDetails
						tradeDetails[tradeCount++] = temp;
						sell.qty = 0;
		
						//the current item is completely sold so we break out of the loop
						break;
          			}
        		}
      		}
      		sendmessage(tempSockFD, "ACCEPTED\n\n");
    	}
    
    
    	else if (strcmp(action, "B") == 0) 	// Client requests for buying an item
    	{
      		//getting poassword and user id
      		char *pass = requestmessage[1];
      		char *uid = requestmessage[0];
      		struct authClientDetails authcheck = authCred("", pass, uid, 'i');
      		char username[50];
      		strcpy(username, authcheck.name);
      		int userid = atoi(uid);
      
		    //validating user
			if ( authcheck.retVal != 1)
	      	{
				sendmessage(tempSockFD, "REJECTED \n\nLogin Unsuccessful. Invalid trader number\n\n");
        		close(tempSockFD);
				continue;
	      	}
		
		    //creating request for the current trade
      		struct tradeItem buy;
      		strcpy(buy.username, username);
      		int item = atoi(requestmessage[3]);
      		buy.itemnumber = item;
      		int qty = atoi(requestmessage[4]);
      		buy.qty = qty;
	      	int unitprice = atoi(requestmessage[5]);
	      	buy.price = unitprice;
	      	buy.id = userid;
	      	buy.type = 'B';
		
  		    //flag to check if the buyer was able to buy all the quantity for that item
      		int check = 0;

	  	    //scanning the sell queue for that item and finding the one with lowest price
      		for (int i = firstLastIndexforSell[item][0]; i != firstLastIndexforSell[item][1]; i = (i + 1) % 1000)
      		{
        		if (sellQueue[i][item].price <= unitprice)
        		{
        	
          		//sell found with quantity more than current buy request
          			if (sellQueue[i][item].qty >= buy.qty)
  					{
            			check = 1;
            
            			//updating quantity fo sell request
            			sellQueue[i][item].qty -= buy.qty;
            			if (sellQueue[i][item].qty == 0)
              				firstLastIndexforSell[item][0] = (firstLastIndexforSell[item][0] + 1) % 1000;
			
				     	//creating trade
	            		struct clientDetails temp;
				    	temp.price = sellQueue[i][item].price;
				    	temp.qty = buy.qty;

				    	strcpy(temp.buyer, username);
				   	    strcpy(temp.seller, sellQueue[i][item].username);

				    	temp.itemnumber = item;
				    	temp.buyerid = buy.id;
				    	temp.sellerid = sellQueue[i][item].id;
				    
				    	//putting trade in tradeDeatails
				    	tradeDetails[tradeCount++] = temp;
				    	break;
		  			}
		  			else
	  				{
		  				//quatity for sell is less than quantity for buy
		    			firstLastIndexforSell[item][0] = (firstLastIndexforSell[item][0] + 1) % 1000;
		    			buy.qty -= sellQueue[i][item].qty;
		    
		    			//creating trade for the quantity that can be bought at once
		    			struct clientDetails temp;
	    				temp.price = sellQueue[i][item].price;
		    			temp.qty = sellQueue[i][item].qty;

	    				strcpy(temp.buyer, username);
		    			strcpy(temp.seller, sellQueue[i][item].username);

		    			temp.itemnumber = item;
		    			temp.buyerid = buy.id;
		    			temp.sellerid = sellQueue[i][item].id;
		    
		    			//updating tradeDetails
		    			tradeDetails[tradeCount++] = temp;
		  		 	}
	       		 }
	      	 }
	      
	      	//if some quantity could not be bought
			if (check == 0)
				queueinsert(buy);
			sendmessage(tempSockFD, "ACCEPTED\n\n");
		}
	    // Client requests for viewing order
		else if (strcmp(action, "VO") == 0)
	    {
			//getting user id and password of the trader
	      	char *uid = requestmessage[0];
	      	char *pass = requestmessage[1];
	      	char username[50];
	      		
			struct authClientDetails authcheck = authCred("", pass, uid, 'i');
	      	int validation = authcheck.retVal;
	      		
			strcpy(username, authcheck.name);

		  	//validating the user
	      	if (validation != 1)
	      	{
				sendmessage(tempSockFD, "REJECTED \n\n Service cannot be processed\n\nInvalid trade number\n\n");
				close(tempSockFD);
	       		continue;
	      	}
	      	sendmessage(tempSockFD, "ACCEPTED\n\n");

    		//contains message to be sent to the client
      		char ret[1000] = "\0";
      		sprintf(ret, "Best Buy/Sell Details---\n");
			sprintf(ret +  strlen(ret), "TYPE  | ITEM ID | QUANTITY | PRICE  | REMARKS                      |\n");
 
      		//checking for all the items
      		for (int i = 0; i < 10; i++)
      		{
        		sprintf(ret + strlen(ret), "\nSell  | %d       ", i);
             
        		//sell available
        		//since the queue is sorted in incresing order of price, the first indesx will be the one with least price
        		if (firstLastIndexforSell[i][0] != firstLastIndexforSell[i][1])
        		{
          			sprintf(ret + strlen(ret), "| %d        | %d      | %31s|\n", sellQueue[firstLastIndexforSell[i][0]][i].qty,sellQueue[firstLastIndexforSell[i][0]][i].price, " ");
        		}
        
        		//sell not available
        		else
          			strcpy(ret + strlen(ret), "|     -    |    -   | No sell available for this item|\n");
		
				//checking for buy
				sprintf(ret + strlen(ret), "\nBuy   | %d       ", i);
		
	       			 //buy available
				if (firstLastIndexforBuy[i][0] != firstLastIndexforBuy[i][1])
				{
		 			//since the queue is sorted in incresing order of price, the last indesx will be the one with max price
		  			int maxprice = buyQueue[(1000 + firstLastIndexforBuy[i][1] - 1) % 1000][i].price;
		  			int maxprice_quantity = buyQueue[(1000 + firstLastIndexforBuy[i][1] - 1) % 1000][i].qty;
		  			for (int j = firstLastIndexforBuy[i][0]; j != firstLastIndexforBuy[i][1]; j = (j + 1) % 1000)
		  			{

		    				if (maxprice == buyQueue[j][i].price)
		    				{
		      					maxprice_quantity = buyQueue[j][i].qty;
		      					break;
		    				}
		  			}
					sprintf(ret + strlen(ret), "| %d        | %d      | %31s|\n", maxprice_quantity,  buyQueue[(1000 + firstLastIndexforBuy[i][1] - 1) % 1000][i].price, " ");
		  			
				}
		
				//buy not available
				else
					strcpy(ret + strlen(ret), "|     -    |    -   | No buy available for this item |\n");
	      		}
	      		sendmessage(tempSockFD, "ACCEPTED\n\n");
	      		sendmessage(tempSockFD, ret);
	    	}
		    else if (strcmp(action, "VT") == 0)   // Client requests for viewing its trade history
	    	{
	      		//getting user id and password of the trader
	      		char *uid = requestmessage[0];
	      		char *pass = requestmessage[1];
	      		char username[50];
	      		
			    struct authClientDetails authcheck = authCred("", pass, uid, 'i');
	      		int validation = authcheck.retVal;
	      		
				strcpy(username, authcheck.name);

		  		//validating the user
	      		if (validation != 1)
	      		{
					sendmessage(tempSockFD, "REJECTED \n\nService cannot be processed\n\nInvalid trade number\n\n");
        			close(tempSockFD);
	       			continue;
	      		}
	      		sendmessage(tempSockFD, "ACCEPTED\n\n");

	      		int userid = atoi(uid);
	      
	      		//message to be sent to the client
	      		char ret[1000];
				sprintf(ret, "------------------------------------------------------------------------------\n");
	      		sprintf(ret + strlen(ret), "|SELLER      |BUYER      |ITEM   |QUANTITY  |PRICE  |BUYER_ID  |SELLER_ID\n");
				sprintf(ret + strlen(ret), "------------------------------------------------------------------------------\n");

	      		sendmessage(tempSockFD, ret);
		
		  		// for all the trades , we check if the trader is a buyer or a seller
		  		//and then we add that trade to the trade datails of current trader
	      		for (int i = 0; i < tradeCount; i++)
	      		{
					if (tradeDetails[i].buyerid == userid || tradeDetails[i].sellerid == userid)
					{
			  			sprintf(ret, "%9s%11s%10d%9d%8d%9d%12d\n", tradeDetails[i].seller, tradeDetails[i].buyer, tradeDetails[i].itemnumber, tradeDetails[i].qty, tradeDetails[i].price, tradeDetails[i].buyerid, tradeDetails[i].sellerid);
			  			sendmessage(tempSockFD, ret);
					}
	      		}
	    	}
	    	
			close(tempSockFD);
	  	}

 		return 0;
}

//---------------------- Sending message from server to client and handeling error
void sendmessage(int sock, char *textMessage)
{
 	if (write(sock, textMessage, strlen(textMessage)) == -1)
  	{
    		printf("Server Error: Message not sent !!");
  	}
}

//---------------------- Function to perform authorization of client credentials
struct authClientDetails authCred(char *username, char *password, char *uID, char ch)
{
	FILE *filePtr = fopen("credentials.txt", "r");
	struct authClientDetails authResult;

	if(!filePtr)
	{
		printf("ERROR in opening file- credentials.txt\n");
  		return authResult;
  	}

  	char temp[100];
  	
 
  	for(int i = 0; fgets(temp, sizeof(temp), filePtr) != NULL; i++)
  	{
  		strcpy(credentials[i], temp);
  		credentials[i][strlen(credentials[i]) - 1] = '\0';
  	}
  	fclose(filePtr);
  
  	for (int i = 0; i < 5; i++)
  	{
  		char *id = strtok(credentials[i], " ");
  		char *name = strtok(NULL, " ");
  		char *pass = strtok(NULL, "\0");

		switch(ch)
		{
			case 'n':
				if (strcmp(username, name) == 0 && strcmp(password, pass) == 0)
		    		{
		      			authResult.retVal = 1;
		      			strcpy(authResult.name, name);
		      			authResult.number = atoi(id);
		      			return authResult;
		    		}
		    		else if (strcmp(username, name) == 0 && strcmp(password, pass) != 0)
		    		{
		      			authResult.retVal = 2;
		      			strcpy(authResult.name, name);
		      			authResult.number = atoi(id);
		      			return authResult;
		    		}
				break;
			case 'i'://id and password both matched
			    	if (strcmp(uID, id) == 0 && strcmp(password, pass) == 0)
			    	{
			      		authResult.retVal = 1;
				        strcpy(authResult.name, name);
				      	authResult.number = atoi(id);
				      	return authResult;
			    	}
			    
			    	//id matched, password not matched
			    	else if (strcmp(uID, id) == 0 && strcmp(password, pass) != 0)
			    	{
			      		authResult.number = atoi(id);
			      		strcpy(authResult.name, name);
			      		authResult.retVal = 2;
			     		return authResult;
			    	}
				break;
		}
  		
  	}
	switch(ch)
	{
		case 'i' : authResult.number = -1;
  			   strcpy(authResult.name, "ID not in Database");
  			   authResult.retVal = 3;
			   break;
		case 'n' : authResult.retVal = 3;
  			   strcpy(authResult.name, "Name not in database");
  			   authResult.number = -1; 
			   break;	
	}	
  	
  	return authResult;
}


//---------------------- Inserting buy and sell requests into respective queues and eviction done in FCFS manner
void queueinsert(struct tradeItem req)
{
	int itemno = req.itemnumber; // Get item ID

  	if (req.type == 'B')   //Buy request
  	{
    		// Getting the starting and ending index of that item in the queue
    		int *tail = &(firstLastIndexforBuy[itemno][1]);
    		int *head = &(firstLastIndexforBuy[itemno][0]);
    		int i = *head;
    
    		// No request in buy queue for that item
    		if (*head == *tail)
    		{
      			buyQueue[*head][itemno] = req;
    		}
    		else
    		{
    			//finding the buy request offering higher price than current price
      			for (i = *head; i < *tail; i = (i + 1) % 1000)
      			{
        			if (buyQueue[i][itemno].price > req.price)
          			break;
     			}
      
      			//this temporary variable is used for shifting the items in queue to one down
      			//making place for the current element to insert
      			struct tradeItem forswap = req;

			//inserting the current buy request and shifting the remaining requests down
      			for (int j = i; j != *tail; j = (j + 1) % 1000)
      			{
        			struct tradeItem temp;
        			temp = buyQueue[j][itemno];
        			buyQueue[j][itemno] = forswap;
        			forswap = temp;
      			}

      			buyQueue[*tail][itemno] = forswap;
    		}

		*tail = (*tail + 1) % 1000;
  	}
	else if (req.type == 'S')          // Sell request
 	{
   		// Getting the starting and ending index of that item in the queue
   		int *head = &(firstLastIndexforSell[itemno][0]);
    		int *tail = &(firstLastIndexforSell[itemno][1]);
    		int i = *head;
    
    		// No sell request for that item in the queue
    		if (*head == *tail)
    		{
      			sellQueue[*head][itemno] = req;  //Add the request into the queue
    		}
    		else
    		{
    			//finding the position to insert the current sell request
      			for (i = *head; i < *tail; i = (i + 1) % 1000)
      			{
      				//position to insert found
        			if (sellQueue[i][itemno].price > req.price)
          				break;
      			}
      
      			//used for shifting the requests after putting the current request
      			struct tradeItem forswap = req;

	  		//inserting the current request and shifting the remaining requests down
      			for (int j = i; j != *tail; j = (j + 1) % 1000)
      			{
        			struct tradeItem temp = sellQueue[j][itemno];
        			sellQueue[j][itemno] = forswap;
        			forswap = temp;
      			}

      			sellQueue[*tail][itemno] = forswap;
    		}
		*tail = (*tail + 1) % 1000;
  	}
}

