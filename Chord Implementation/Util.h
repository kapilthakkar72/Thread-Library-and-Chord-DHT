#include <algorithm>

void processMessage(char* buffer);
void fixFingers();

/**
This function will return my IPv4 IP Address
*/
string getMyIPAdress()
{
	struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    int i=0;
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
           
            if (i==1)
            {
              return addressBuffer;
            }
             i++;
        } 
    }

    return NULL;
}
/**
This Function will generate random keys based on the current time.
*/
int getMyKeyValue()
{
	// get current time for id

    time_t epoch_time;
    struct tm *tm_p;
    epoch_time = time( NULL );
    tm_p = localtime( &epoch_time );

    srand (time(NULL));

   	return (tm_p->tm_sec*1000 + (rand()%10)*100 + tm_p->tm_min) ;
}

/**
a belongs to (b,c]
*/     
int belongsOC (int a,int b,int c)
{
	if(b<c)
  {
    if(b<a && a<=c  )
    {
       return 1;
    }           
    else
    {
      return 0;
    }
  }        
  else
  {
    if(b==c)
    {
      return 1;
    }       
    else if(a> b || a<=c)
    {
      return 1;
    }              
  }     
  return 0;
}

/**
a belongs to [b,c)
*/       
int belongsCO(int a,int b,int c)
{
	if(b<c)
        if(b<=a && a<c )
            return 1;
        else
            return 0;
    else
        if(b==c)
            return 1;
        else
            if(a>= b || a<c)
           		return 1;
        
    return 0;
}        



/**
a belongs to (b,c)
*/
         
int belongsOO(int a,int b,int c)
{
	if(b<c)
        if(b<a && a<c )
            return 1;
        else
            return 0;
    else
        if(b==c)
            return 1;
        else
            if(a> b || a<c)
           		return 1;
        
    return 0;
}
/**
This function will split the string into multiple strings by making use of delimiter
*/
void SplitString(string message,string delimiter,string result[])
{
  message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
	string token, mystring(message);
	int i=0;
	while(token != mystring){
  token = mystring.substr(0,mystring.find_first_of(delimiter));
  mystring = mystring.substr(mystring.find_first_of(delimiter) + 1);
  
  result[i]=token;
  i++;
	}
}


/**
This function will return the row of finger table, of which netry is the closest to the passed key as an argumnt.
*/
int closestPrecedingFinger(int key)
{
	int i;
	for(i=NumberFingerEntries;i>=1;i--)
	{
		if(belongsOO(RingInfo.finger[i].keyOfNode,MyKey,key))
		{
			return i;
		}
	}
	return 0;
}


/**
This function will create the ring and will initialise chord network.
*/
void createRing()
{
	RingInfo.predIP=MyIP;
	RingInfo.predPort=MyPort;
	RingInfo.predKey=MyKey;
	
	for(int i= 1;i<=NumberFingerEntries;i++)
	{
		RingInfo.finger[i].ip=MyIP;
		RingInfo.finger[i].port=MyPort;
		RingInfo.finger[i].keyOfNode=MyKey;
	}
}


/**
This function will be called whenevr any new node will join the network.
*/
void joinToNetwork(string useIP,int usePort)
{
	char MyPortString[5];
	snprintf(MyPortString,5,"%d",MyPort); 

	char MyKeyString[6];
	snprintf(MyKeyString,6,"%d",MyKey); 

	// get successor
	
	string message= MyIP + " " + MyPortString + " successor " +MyKeyString +" finger 1";

	sendMessage(useIP,usePort,message);
	
	// get predecessor

	// wait till it gets its successor
	
	while(RingInfo.finger[1].port == -1);

	message= MyIP + " " + MyPortString + " request join predecessor" ;
	sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);

	
	
	// Fill Finger Table Entries
	for(int i= 2;i<=NumberFingerEntries;i++)
	{
		// Convert Key to string

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
      else
      {
        message= MyIP + " " + MyPortString + " successor " +keyString +" finger "+ iString;
        sendMessage(useIP,usePort,message);
      }		
	}	
	// change predecessor of your successor

	message= "change predecessor " + MyIP + " " + MyPortString + " " + MyKeyString;
	sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);

	// change successor of your predecessor
	while(RingInfo.predPort==-1);
	message= "change successor " + MyIP + " " + MyPortString + " " + MyKeyString;
	sendMessage(RingInfo.predIP,RingInfo.predPort,message);

  message="redistribute request";
  sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);

  while(!keyRedistributed);

  if((RingInfo.predKey == RingInfo.finger[1].keyOfNode) && (RingInfo.predKey !=MyKey))
  {
    //I am the second node First time started.
    fixFingers();
    sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,"fixfinger");
  }
}


/**
This function will work as a server to listen on a port. Recieve Message and send for processing.
*/
void acceptConnection()
{
  struct sockaddr_in cl_addr;
  int len,newsockfd,ret;
  int CLADDR_LEN=100;
  char clientAddr[CLADDR_LEN];

  while(1)
  {
    
    len = sizeof(cl_addr);  
    newsockfd = accept(socketID,(struct sockaddr*)&cl_addr, (socklen_t*)&len);  
    
    if (newsockfd < 0) 
    {  
         continue;
    }   
  
    inet_ntop(AF_INET, &(cl_addr.sin_addr), clientAddr, CLADDR_LEN);  
    
    // Accept Message
  
    char buffer[BUF_SIZE];  
    memset(buffer, 0, BUF_SIZE); 
    ret = recvfrom(newsockfd, buffer, BUF_SIZE, 0, NULL, NULL);    
    //printf("Message recieved\n");
     
    if (ret < 0) 
    {    
       //printf("Error in receiving data!\n");
       continue;      
    } 
    else 
    {  
       processMessage(buffer); 
    }

    close(newsockfd);
  }
}


/**
This function will work as a client and will help to send messages to other nodes.
*/
void* sendMessage(string ip_address,int port,string message)
{
	if(ip_address==MyIP && port==MyPort)
	{
		processMessage((char*)message.c_str());
	}
	else
	{
  	int sockfd, ret;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);    
  if (sockfd < 0) 
  {    
    printf("Error creating socket!\n");    
    return NULL;
  }    
  //printf("Socket created...\n");   

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));    
  addr.sin_family = AF_INET;    
  addr.sin_addr.s_addr = inet_addr(ip_address.c_str());  
  addr.sin_port = port;    

  ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));    
  if (ret < 0) 
  {    
  	time_t timer;
    char buffer[25];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    //strftime(buffer, 25, "%Y:%m:%d%H:%M:%S:%SSS", tm_info);
    //printf("Error connecting to the server! Ip:%s ,Port:%d\n",ip_address.c_str(),port);
    
    while(ret<0)
    {
     	ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)); 
    }
  }    

  ret = sendto(sockfd, message.c_str(), sizeof(message)*message.length(), 0, (struct sockaddr *) &addr, sizeof(addr));    
  if (ret < 0) 
  {    
      printf("Error sending data!\n\t-%s", message.c_str());
      while(ret<0)
      {
        ret = sendto(sockfd, message.c_str(), sizeof(message)*message.length(), 0, (struct sockaddr *) &addr, sizeof(addr)); 
      }
  }
    
    close(sockfd);  
    return NULL;
}
}   


/**
This function will be used to compute hash for a given key in string format.
*/ 

int computeHash(string keyData)
{
  long hash=0;
  int start=45;
  
  for (int i = 0; i < keyData.size(); i++)
  {
    if(keyData[i]!='\n')
    {
      hash=hash+(keyData[i]*start);
      start=start+15;
    }
  }
 return hash%((int)pow(2,16));
}


/**
This function will help to start process of put (key,value) pair to corresponding node.
*/

void putMessage(string key,string value)
{
  int hash=computeHash(key);
 
  if(belongsOC(hash,RingInfo.predKey,MyKey))
  {
    database[key]=value;
    printf("Yo..!!! Chocolate is mine... (%s->%s) %d\n", key.c_str(),value.c_str(),hash);
  }
  else
  {
    char MyPortString[5];
    snprintf(MyPortString,5,"%d",MyPort);  

    char hashKey[6];
    snprintf(hashKey,6,"%d",hash);
    string message=MyIP+" "+MyPortString+" successor "+hashKey+" put "+key+" "+value;
    sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);
  }
}



/**
This function will help to process get value of a given key.
*/
void getMessage(string key)
{
  int hash=computeHash(key);
  if(belongsOC(hash,RingInfo.predKey,MyKey))
  {
    printf("Key:%s Value: %s\n",key.c_str(),database[key].c_str());
  }
  else
  {
    char MyPortString[5];
    snprintf(MyPortString,5,"%d",MyPort);  

    char hashKey[6];
    snprintf(hashKey,6,"%d",hash);
    string message=MyIP+" "+MyPortString+" successor "+hashKey+" value "+key;
    sendMessage(RingInfo.finger[1].ip,RingInfo.finger[1].port,message);
  }
}


/**
This function will provide all information stored at this node.
*/
string dumpNode()
{
        char MyKeyString[6];
        snprintf(MyKeyString,6,"%d",MyKey);

        char MyPortString[5];
        snprintf(MyPortString,6,"%d",MyPort);

        char successorPort[5];
        snprintf(successorPort,6,"%d",RingInfo.finger[1].port);

        char predecessorPort[6];
        snprintf(predecessorPort,6,"%d",RingInfo.predPort);

        char successorKey[6];
        snprintf(successorKey,6,"%d",RingInfo.finger[1].keyOfNode);

        char predecessorKey[6];
        snprintf(predecessorKey,6,"%d",RingInfo.predKey);
        
        string message="My IP Address "+ MyIP + "\nMy Key -> " +string(MyKeyString)+ "\nMy Port -> " + string(MyPortString) + "\nMy Successor -> "+ RingInfo.finger[1].ip +":"+ string(successorPort)+"    Key:" + successorKey +"\nMy Predecessor "+RingInfo.predIP+":"+ predecessorPort +"    Key:"+predecessorKey+"\nMy Finger Table:\n\n";

        message+="Key \t Interval \t  Node\n";
        message+="========================================================\n";

        for (int i = 1; i <= NumberFingerEntries; ++i)
        {
          char tempKey[6];
          snprintf(tempKey,6,"%d",RingInfo.finger[i].key);

          char tempEnd[6];
          snprintf(tempEnd,6,"%d",RingInfo.finger[i].end);

          char tempPort[6];
          snprintf(tempPort,6,"%d",RingInfo.finger[i].port);

          char tempKeyOfNode[6];
          snprintf(tempKeyOfNode,6,"%d",RingInfo.finger[i].keyOfNode);

          message+= "\n"+string(tempKey)+" \t ["+string(tempKey)+","+string(tempEnd)+")    "+RingInfo.finger[i].ip+":" +string(tempPort)+" Key->"+ string(tempKeyOfNode);
        }
        message+="\n========================================================\n";

        return message;
}


/**
This function will return all the (key,value) pair stored at this node.
*/
string myKeys()
{
  char MyKeyString[6];
  snprintf(MyKeyString,6,"%d",MyKey);

   string message="";
   char MyPortString[5];
   snprintf(MyPortString,5,"%d",MyPort);
   if(database.empty())
   {
        message=message+"No keys present at this node ("+ MyIP +"," +MyPortString +") with key "+MyKeyString+".\n";
   }
   else
   {

      message += "Keys present at this node ("+ MyIP +"," +MyPortString +"):: with key "+MyKeyString+".\n";
      for (map<string,string>::iterator it=database.begin(); it!=database.end(); ++it)
      {
        char MyKeyHash[7];
        snprintf(MyKeyHash,7,"%d",computeHash(it->first));
         message=message+ MyKeyHash+" "+ it->first+"=>" +it->second +"\n";
         //std::cout << it->first << " => " << it->second << '\n';
      }  
   }
   return message;
}