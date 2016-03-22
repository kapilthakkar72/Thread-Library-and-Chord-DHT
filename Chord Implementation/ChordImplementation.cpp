#include "stdio.h"  
#include "stdlib.h"  
#include "sys/types.h"  
#include "sys/socket.h"  
#include "string.h"  
#include "netinet/in.h"  
#include "MyThread.h"  
#include <arpa/inet.h>
#include <errno.h> 
#include <math.h>
#include <unistd.h>
#include <map>
#include <ifaddrs.h>

using namespace std;

#define BUF_SIZE 2000
#define NumberFingerEntries 16

/**
Each instance of this class will represent one row of finger table.
*/
class FingertableEntry
{
	public:
	int key;
	int end;
	string ip;
	int port;
	int keyOfNode;
};


/**
Each node will have one instance of this to store finger table, its successor and predecessor information.
*/
class FingerTable
{
	public:
	FingertableEntry finger[NumberFingerEntries+1];
	string predIP;
	int predPort; 
	int predKey;
	
	FingerTable()
	{
		predIP="";
		predPort=-1;
	}


    /**
        Initialisation with default values.
    */

	void initMyFingerTable(int MyKey)
	{
		for(int i=1;i<=NumberFingerEntries;i++)
		{
			finger[i].key = (int)(MyKey + pow(2,i-1))%(int)pow(2,16);
			finger[i].end = (int)(MyKey + pow(2,i))%(int)pow(2,16);
			finger[i].port = -1;
		}
	}
};

// Socket ID

int socketID;

string MyIP;
int MyPort;
int MyKey;

FingerTable RingInfo;


/**
To store Key Value pair.
*/
map<string,string> database;

int keyRedistributed;

void* sendMessage(string ip_address,int port,string message);

#include "Util.h"


/**
This will be called periodically to correct finger table entries whenever new node will join the network.
*/

void fixFingers()
{
	int i;
	
    for(i=2;i<=NumberFingerEntries;i++)
	{
		char MyPortString[5];
		snprintf(MyPortString,5,"%d",MyPort); 

		char keyString[6];
		snprintf(keyString,6,"%d",RingInfo.finger[i].key);

		char iString[3];
		snprintf(iString,3,"%d",i);
		
		if(belongsOC(RingInfo.finger[i].key,MyKey,RingInfo.finger[1].keyOfNode))
		{
			RingInfo.finger[i].ip = RingInfo.finger[1].ip;
			RingInfo.finger[i].port = RingInfo.finger[1].port;
			RingInfo.finger[i].keyOfNode =RingInfo.finger[1].keyOfNode;
		}
		else if(belongsOC(RingInfo.finger[i].key,RingInfo.predKey,MyKey))
        {
            RingInfo.finger[i].ip = MyIP;
            RingInfo.finger[i].port = MyPort;
            RingInfo.finger[i].keyOfNode =MyKey;
        }
        else
    	{
            //sleep(2);
            //printf("finger: %d (%d,%d)\n",RingInfo.finger[i].key,RingInfo.predKey,RingInfo.finger[1].keyOfNode);
            string message= MyIP + " " + MyPortString + " successor " +keyString +" finger "+ iString;
    		sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);
		}			
	}
}


/**
To accept commands from the user.
*/
void userInteraction()
{
    
    char buffer[BUF_SIZE];
    int createdNetwork=0;
    while(1)
    {
    	int flagOfFinger=1;

        printf("\n\nChord_Terminal $ ");

        string inputSplitted[10];

        fflush(stdin);
        buffer[0]='\0';
        while(fgets(buffer, BUF_SIZE, stdin)==NULL);
        string input(buffer);
        string command(buffer);

        string delimiter = " ";
        string token = input.substr(0, input.find(delimiter));

        SplitString(input," ",inputSplitted); 


        if(command=="help\n")
        {
            printf("create                   : Creates the ring.\n");
            printf("join <IP port>           : Join the ring with IP address and port.\n");
            printf("quit                     : Shuts down the ring.\n");
            printf("put <key> <value>        : Insert the given <key,value> pair in the ring.\n");
            printf("get <key>                : Returns the value corresponding to the key, if one was previously inserted in the node.\n");
            printf("finger                   : A list of addresses of nodes on the ring.\n");
            printf("successor                : An address of the next node on the ring.\n");
            printf("predecessor              : An address of the previous node on the circle\n");
            printf("dump                     : Display all information pertaining to calling node.\n");
            printf("dumpall                  : All information of all the nodes.\n");
            printf("dumpaddr <address port>  : Display all information pertaining to node at (address,port).\n");
            printf("allkeys                  : (Key,Value) pairs present at each node.\n");
            printf("mykeys                   : (Key,Value) pairs present at this node.\n");
        }
        else if(command=="allkeys\n")
        {
            if(RingInfo.predKey == MyKey)
            {
                // I am the only node in the network 
                printf("%s\n",myKeys().c_str());
            }
            else
            {
                char MyPortString[6];
                snprintf(MyPortString,6,"%d",MyPort);

                printf("%s\n",myKeys().c_str());
                string message=MyIP+ " "+ MyPortString + " allkeys";
                sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);
            }
        }
        else if(command=="successor\n")
        {
            printf("\nMy Successor : IP - %s \t Port - %d \t Key - %d\n",RingInfo.finger[1].ip.c_str(),RingInfo.finger[1].port,RingInfo.finger[1].keyOfNode);
        }
        else if(command=="predecessor\n")
        {
            printf("\nMy Predecessor : IP - %s \t Port - %d \t Key - %d\n",RingInfo.predIP.c_str(),RingInfo.predPort,RingInfo.predKey);
        }
        else if(command=="finger\n")
        {
            char MyPortString[6];
            snprintf(MyPortString,6,"%d",MyPort);

            printf("fingerNode %s %d\n",MyIP.c_str(),MyPort);
            string message= MyIP +" "+MyPortString +" fingerNode";
            sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);
        }
        else if(command=="dumpall\n")
        {
           	if(RingInfo.predKey == MyKey)
        	{
        		// I am the only node in the network 
        		printf("%s\n",dumpNode().c_str());
        	}
        	else
        	{

            char MyPortString[6];
            snprintf(MyPortString,6,"%d",MyPort);

            printf("%s\n",dumpNode().c_str());
            string message=MyIP+ " "+ MyPortString + " dumpall";
            sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);
        	}
        }
        else if(command=="quit\n")
        {
        	if(RingInfo.predKey == MyKey)
          {
            // I am the only node in the network 
            close(socketID);
            exit(1);
          }
          else
          {
            char MyKeyString[6];
            snprintf(MyKeyString,6,"%d",MyKey);


            string message=string(MyKeyString) +" quit";
            sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);
            //printf("quit request Sent to successor %s,%d\n",RingInfo.finger[1].ip.c_str(),RingInfo.finger[1].port);
          }
        }

		else if(command == "create\n")
		{
			createRing();
			printf("\nNetwork Created With IP Address: %s and Port: %d\n",MyIP.c_str(),MyPort);	
			createdNetwork=1;
		}
        else if(command == "mykeys\n")
        {
            printf("%s\n",myKeys().c_str());
        }
        else if(token == "put")
        {
          putMessage(inputSplitted[1],inputSplitted[2]);
        }
        else if(token == "get")
        {
            //get key
          getMessage(inputSplitted[1]);
          
        }
        else if(token == "join")
        {
			// extract ip

			string useThisNodeIP = inputSplitted[1];

			// extract port

			int useThisNodePort = atoi(inputSplitted[2].c_str());

			printf("\nJoining to Network...%s,%d\n",useThisNodeIP.c_str(),useThisNodePort);
			
			joinToNetwork(useThisNodeIP,useThisNodePort);

			printf("\nRequests sent to join. Initialising...\n");
		}
        else if(command == "dump\n")
        {
        	printf("%s",dumpNode().c_str());
		}
        else if(token=="dumpaddr")
        {
            string ip=inputSplitted[1];
            int port=atoi(inputSplitted[2].c_str());

            char MyPortString[6];
            snprintf(MyPortString,6,"%d",MyPort);

            sendMessage(ip,port,MyIP+" "+ MyPortString+" dumpaddr request");
        }
    }
}


/**
Process the recieved message and take appropriate action.
*/
void processMessage(char* buffer)
{
	// My Port

	char MyPortString[5];
	snprintf(MyPortString,5,"%d",MyPort); 

	char MyKeyString[6];
	snprintf(MyKeyString,6,"%d",MyKey); 


	//First Split the Message

		string splittedMessage[1000];
		string input(buffer);
        string delimiter = " ";
        SplitString(input," ",splittedMessage);

	// Fetch IP and Port to which we need to reply

        string replyToIP = splittedMessage[0];
        int replyToPort = atoi(splittedMessage[1].c_str());

     // When Node Joins The network

        if(splittedMessage[2]=="fingerNode")
        {
            //printf("It came here\n");
            if(splittedMessage[0] == MyIP && atoi(splittedMessage[1].c_str())==MyPort)
            {
                // do nothing
            }
            else
            {
                //printf("Replying for fingerNode\n");
                string message1="fingerNode "+MyIP + " "+MyPortString;
                sendMessage(replyToIP,replyToPort,message1);
                sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,input);
            }
        }
        if(splittedMessage[0]=="fingerNode")
        {
          //printf("fingerNode Recieved\n");
          printf("%s\n",input.c_str());
        }
        
        if(splittedMessage[2]=="dumpaddr")
        {
            if(splittedMessage[3]=="request")
            {
                sendMessage(replyToIP,replyToPort,"dumpaddr reply "+dumpNode());  
            }
        }
        else if(splittedMessage[0]=="dumpaddr")
        {
          if(splittedMessage[1]=="reply")
          {
            printf("%s\n",input.c_str());
          }
        }

        // Message to quit the ring
        else if(splittedMessage[1]=="quit")
        {
        	printf("My key recahed string:%s integer:%d Mykey:\n",splittedMessage[0].c_str(),atoi(splittedMessage[0].c_str()),MyKey);
        	if(atoi(splittedMessage[0].c_str())==MyKey)
        	{
        		// message ne ring ghum li
        		//printf("Finally quit came to start \n");
        		close(socketID);
        		exit(1);
        	}
        	else
        	{
        		// send message to my succesor
        		//printf("Send quit request to successor %s,%d\n",RingInfo.finger[1].ip.c_str(),RingInfo.finger[1].port);
        		sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,input);
        		//printf("quit request Sent to successor %s,%d\n",RingInfo.finger[1].ip.c_str(),RingInfo.finger[1].port);
        		//exit
        		exit(1);
        	}
        }
        if(splittedMessage[2]=="dumpall")
        { 
            if(splittedMessage[0] == MyIP && atoi(splittedMessage[1].c_str())==MyPort)
            {
                // do nothing
            }
            else
            {
                //printf("Replying for dump\n");
                string message1="dumpall "+dumpNode();
                sendMessage(replyToIP,replyToPort,message1);
                sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,input);
            }
        }
        if(splittedMessage[0]=="dumpall")
        {
          //printf("Dump Recieved\n");
          printf("%s\n",input.c_str());
        }
        if(splittedMessage[2]=="allkeys")
        { 
            if(splittedMessage[0] == MyIP && atoi(splittedMessage[1].c_str())==MyPort)
            {
                // do nothing
            }
            else
            {
                //printf("Replying for allkeys\n");
                string message1="allkeys "+myKeys();
                sendMessage(replyToIP,replyToPort,message1);
                sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,input);
            }
        }
        if(splittedMessage[0]=="allkeys")
        {
          printf("%s\n",input.c_str());
        }
    //Serve the request for redistribute
    if(input=="fixfinger")
    {
      fixFingers();
      sleep(1);
      sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,"fixfinger");
    }

    if(splittedMessage[0]=="redistribute" && splittedMessage[1]=="request")
    {
        int returningKeys=0;
        string allKeys="";
        for (map<string,string>::iterator it=database.begin(); it!=database.end(); ++it)
        {
            //find the value of hash
            int hash=computeHash(it->first);
            if(!belongsOC(hash,RingInfo.predKey,MyKey))
            {
              allKeys+=" "+it->first+" "+it->second;
              database.erase(it->first);
              returningKeys++;
            }
        }

        char returningKeysString[5];
        snprintf(returningKeysString,5,"%d",returningKeys);


        string message="redistribute reply "+ string(returningKeysString) + allKeys;  
        sendMessage(RingInfo.predIP,RingInfo.predPort,message); 
        //printf("Request served %s\n", message.c_str());  
    }

    // serve the reply for redistribute
    if(splittedMessage[0]=="redistribute" && splittedMessage[1]=="reply")
    {
        //printf("Reply Received : %s\n",input.c_str());
        keyRedistributed=1;
        int noOfKeys=atoi(splittedMessage[2].c_str());

        for(int i=0;i<noOfKeys;i++)
        {
            string key=splittedMessage[3+i*2];
            string value=splittedMessage[4+i*2];
            database[key]=value;
        }
    }

		//if some new node is saying to change my values
		if(splittedMessage[0]=="change")
		{
			if(splittedMessage[1]=="successor")
			{
				RingInfo.finger[1].ip=splittedMessage[2];
				RingInfo.finger[1].port=atoi(splittedMessage[3].c_str());
				RingInfo.finger[1].keyOfNode=atoi(splittedMessage[4].c_str());
			}
			else if(splittedMessage[1]=="predecessor")
			{
				RingInfo.predIP=splittedMessage[2];
				RingInfo.predPort=atoi(splittedMessage[3].c_str());
				RingInfo.predKey=atoi(splittedMessage[4].c_str());
			}
		}

		// Serve the Request
		else if(splittedMessage[2] == "request")
		{
			if(splittedMessage[3] == "join")
			{
				if(splittedMessage[4] == "predecessor")
				{
					// Send My Predecessor
					char predPortString[5];
					snprintf(predPortString,5,"%d",RingInfo.predPort); 

          char predKeyString[6];
          snprintf(predKeyString,6,"%d",RingInfo.predKey); 

					string message= MyIP + " " + MyPortString + " reply join predecessor " + RingInfo.predIP  + " " + predPortString+" "+ predKeyString;
					// send reply
					sendMessage(replyToIP,replyToPort,message);	
				}
			}
		}
		else if(splittedMessage[2]=="successor")
		{
			int keyToBeProcessed = atoi(splittedMessage[3].c_str());

			if(belongsOC(keyToBeProcessed,RingInfo.predKey,MyKey))
			{
				
				// (key,value) pair is with me
				// find and reply				
                if(splittedMessage[4]=="value")
                {		
                    string message= splittedMessage[2]+" "+splittedMessage[3]+" "+splittedMessage[4]+" "+splittedMessage[5]+" "+database[splittedMessage[5]];
                    sendMessage(replyToIP,replyToPort,message);
                    return;
                }
                else if(splittedMessage[4]=="put")
                {
                    printf("I got request for put\n");
                    database[splittedMessage[5]]=splittedMessage[6];
                    return;
                }

			}
			if(belongsOC(keyToBeProcessed,MyKey,RingInfo.finger[1].keyOfNode))
			{
				// I am the predecessor
				// Return My Successor	
				char SuccPort[5];
				snprintf(SuccPort,5,"%d",RingInfo.finger[1].port); 	

				char SuccKey[6];
				snprintf(SuccKey,6,"%d",RingInfo.finger[1].keyOfNode);

        if(splittedMessage[4]=="value" ||splittedMessage[4]=="put") 
        {
            // (key,value) pair is with my successor. So let it handle this request.
            sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,input);
            return;
        }			

				string message= splittedMessage[2]+" "+splittedMessage[3]+" "+splittedMessage[4]+" "+splittedMessage[5]+" "+RingInfo.finger[1].ip+" "+SuccPort+" "+SuccKey;
				sendMessage(replyToIP,replyToPort,message);
			}
			else
			{
				int row=closestPrecedingFinger(keyToBeProcessed);
				if(row==0)
				{
					//myself
					printf("Not Handled Yet! If You see this print info and workout.\n");
				}
				else
				{
					//row[i]
					sendMessage(RingInfo.finger[row].ip,RingInfo.finger[row].port,input);
				}
			}
		}
		// process the Reply
		if(splittedMessage[2] == "reply")
		{
			if(splittedMessage[3]== "join")
			{
				if(splittedMessage[4]== "predecessor")
				{
					RingInfo.predIP = splittedMessage[5];
					RingInfo.predPort = atoi(splittedMessage[6].c_str());
                    RingInfo.predKey = atoi(splittedMessage[7].c_str());
				}
			}
		}

		// process the reply of successor

		if(splittedMessage[0] == "successor")
		{
			if(splittedMessage[2]=="finger")
			{
				// its reply for finger table entry
				// change entry 
				int entry = atoi(splittedMessage[3].c_str());
				RingInfo.finger[entry].ip=splittedMessage[4];
				RingInfo.finger[entry].port=atoi(splittedMessage[5].c_str());
				RingInfo.finger[entry].keyOfNode=atoi(splittedMessage[6].c_str());
			}
			else if(splittedMessage[2]=="value")
			{
				// it was request to find key value - pair 
				// so display the value
				printf("Key: %s \t Value: %s\n",splittedMessage[3].c_str(),splittedMessage[4].c_str());
			}
		}
}




int main(int argc,char *argv[])
{
	MyIP = getMyIPAdress();

	MyKey = getMyKeyValue();    
    RingInfo.initMyFingerTable(MyKey);

  int port,ret;

  if(argc!=2)
  {
    printf("Please Provide Port Number as a command Line Argument\n");
    return 1;
  }
    
  MyPort= port = atoi(argv[1]);

  // Create Socket
  socketID= socket(AF_INET, SOCK_STREAM, 0);  

  if (socketID < 0) 
  {  
      printf("Error creating socket!\n");  
      exit(1);  
  }

  // Preapring for Binding

  struct sockaddr_in addr;

  memset(&addr, 0, sizeof(addr));  
  addr.sin_family = AF_INET;  
  addr.sin_addr.s_addr = INADDR_ANY;  
  addr.sin_port = port;  
   
  ret = bind(socketID, (struct sockaddr *) &addr, sizeof(addr));  
  if (ret < 0) 
  {  
      printf("Error binding!\n");  
      if( ret == EADDRINUSE )
        {
            printf("the port is not available. already to other process\n");
        } 
        else 
        {
            printf("could not bind to process (%d) %s\n", ret, strerror(ret));
        }
      exit(1);  
  }  

  listen(socketID, 1000);  

  create(acceptConnection);
  create(userInteraction);
  start();
  
  return 1;
}
