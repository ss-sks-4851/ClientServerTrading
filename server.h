/*//////////////////////////////////////////////////
/                                                  /
/                                                  /
*********************STRUCTURES*********************
/                                                  /
/                                                  /
//////////////////////////////////////////////////*/

// Encapsulates the data retrieved based on the login details entered by client
struct authClientDetails
{
	char name[100]; //Client name
	int number; 	//Number of times the client has logged in
  	int retVal;
  	/* --- name and pass matches --> retVal = 1
  	       name and pass does not match --> retVal = 2
	       name not in DB --> retVal = 3
	   --- */
};

// Trade details of any Client
struct clientDetails
{
	char buyer[100]; // Buyer's NAME
  	char seller[100]; // Seller's NAME
  	int buyerid;
  	int sellerid;
  	int itemnumber;
  	int price;
  	int qty;
};

// Details of an entity in the queue
struct tradeItem
{
	char username[100]; // Buyer's or Seller's name
  	int itemnumber; // Item Number or Id
  	int id; // Buyer's or Seller's Id
 	int qty; // Quantity for the item to buy or sell
  	int price; // Item Price
  	char type; // 'B' for Buy, 'S' for Sell
};


/*//////////////////////////////////////////////////
/                                                  /
/                                                  /
***************GLOBAL VARIABLES*********************
/                                                  /
/                                                  /
//////////////////////////////////////////////////*/

// User details of all matched trades
struct clientDetails tradeDetails[1000];

// Buy queue - makes sense when mulitple buy request are present simultaneously
struct tradeItem buyQueue[1000][10];

// Sell queue - makes sense when mulitple sell request are present simultaneously
struct tradeItem sellQueue[1000][10];

// First and Last index of Buy Queue for every item number or id
int firstLastIndexforBuy[10][2] = {0};

// First and Last index of Sell Queue for every item number or id
int firstLastIndexforSell[10][2] = {0};

// Denotes the total number of matched trades
int tradeCount = 0;

// Temperorly storing credentials
char credentials[5][100];



/*//////////////////////////////////////////////////
/                                                  /
/                                                  /
*********************FUNCTIONS*********************
/                                                  /
/                                                  /
//////////////////////////////////////////////////*/

void sendmessage(int sock, char *textMessage);
struct authClientDetails authCred(char *uname, char *ps, char *uid, char ch);
void queueinsert(struct tradeItem req);
