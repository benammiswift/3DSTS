// TSControlValueDebugConsole.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "MainHeader.h" //Main header for all my includes, typedefs etc.


#define PORT        6942
#define BUFSIZE     8192


HMODULE Raildriver;
HMODULE Overlay;
SetRailDriverConnectedType SetRailDriverConnected;
GetControllerListType GetControllerList;
SetControllerValueType SetControllerValue;
GetCurrentControllerValueType GetCurrentControllerValue;
GetLocoNameType GetLocoName;
SetControllerValueType SetRailSimValue;
GetCurrentControllerValueType GetRailSimValue;
GetControllerValueType GetControllerValue;

using namespace std;

//Function prototypes
bool Initialise();
bool LoadDll();
bool GrabControlNames();
bool serialiseControlNames();
bool readstr();
int getIndex( char* K);
void ClearArray();
float CSToF( char* );
bool runCommand(char* cmd);
int printIPAddress();
void readCommand(char* input);
void getAndSendLocoName();

//Globals
char delim[] = "::";
char* CVArray[512] = { NULL };
int CVArraySize = 0;
int ListLength = 0;
const int commandsize = 7;
char* command[commandsize] = { NULL };
char serialised[16386];
SOCKET clientSocket;

int main() { //Main Function

    if (!Initialise()) { //Error Handling here for if Init failed

        cout << "An error occured during initalisation, exiting...";
        return 1;
    }
    puts("TS API initialised");
#ifdef _DEBUG
    puts("This is a debug build");
#endif


    serialiseControlNames();

    //Networking Init
    //----------------------------

    //Init winsock
    puts("Initialising Winsock..");
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    int wsOK = WSAStartup(ver, &wsData);
    if (wsOK != 0)
    {
        puts("Error initialising winsock, exiting...");
        return 1;
    }
    puts("Winsock initialised");

    //Create a socket

    puts("Creating listener socket..");
    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET)
    {
        puts("Cant create a listener socket");
        return 1;
    }
    puts("Listener socket created");

    //Bind an ip address and port to the socket

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(listening, (sockaddr*)&hint, sizeof(hint));

    puts("\nYour IP Addresses\n---------------------------------\n");
    printIPAddress();
    puts("---------------------------------\n\nIf you don't already know your local IP:");
    puts("please attempt to connect with these IPs,\none of them will be your local IP");

    //Tell winsock the socket is for listening
    puts("\nWaiting for client...");
    listen(listening, SOMAXCONN);

    //Wait for connection

    sockaddr_in client;
    int clientSize = sizeof(client);

    clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
    if (clientSocket == INVALID_SOCKET)
    {
        puts("Cant create client socket");
        return 1;
    }
    puts("Created client socket");

    char host[NI_MAXHOST];      //Client's remote name
    char service[NI_MAXHOST];   //Service (ie port the client is connected on

    ZeroMemory(host, NI_MAXHOST);   // use memset(host, 0, NI_MAXHOST); on unix systems
    ZeroMemory(service, NI_MAXHOST);

    if (getnameinfo((sockaddr*)&client, clientSize, host, NI_MAXHOST, service, NI_MAXHOST, 0) == 0)
    {
        printf("%s connected on port %s\n", host, service);
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        printf("%s connected on port %s\n", host, ntohs(client.sin_port));
    }


    // Close listener

    closesocket(listening);

    //End of network init and connection
    char buf[BUFSIZE];

    while (true)
    {
        ZeroMemory(buf, BUFSIZE);

        //Wait for client to send data
        int bytesRecieved = recv(clientSocket, buf, BUFSIZE, 0);
        if (bytesRecieved == SOCKET_ERROR)
        {
            puts("Error in recv(), exiting..");
            break;
        }

        if (bytesRecieved == 0)
        {
            puts("Client disconnected");
            break;
        }
        printf("Recv message : %s\n", buf);

        char* token, *next_token;
        token = strtok_s(buf, COMM_SYNTAX_TERM_MSG, &next_token);

        /* walk through other tokens */
        while (token != NULL) {
            printf("Split off : %s\n", token);
            readCommand(token);
            runCommand((char*)"a");
            token = strtok_s(NULL, COMM_SYNTAX_TERM_MSG, &next_token);;
        }

        //readCommand(buf);
        //runCommand((char*)"a");

        //Echo message to client
        for (int i = 0; i < BUFSIZE; i++)
        {
            if (buf[i] == '\n')
                buf[i] = '\0';
        }
        if (strlen(buf) > 0)
        {
            printf("Recieved message \"%s\", echoing...\n", buf);
        }

        //send(clientSocket, buf, bytesRecieved + 1, 0);
    }
    
    /*
    while (1 == 1) { //Main program loop, only exited by calling the exit command or if the Initialise fails when Update is called

        cout << "\n\n" << "Enter a command : ";

        readstr(); //This function just gets our string

        runCommand((char *)"help");
    }
    */

    cout << "Exiting...";

    return 0;

}

bool Initialise() { //Initialise function, called on launch and when Update terminal function is called forcing a full reinit

    if (!LoadDll()) { return false; } //Run the load DLL function and error out if it failed.

    SetRailDriverConnected(true);

    if (!GrabControlNames()) { return false; };

    cout << "Init Complete\n";

    return true;

}

bool LoadDll() { //This function loads the DLL and binds the function

    //If we're running a debug build then use the hardcoded path to my raildriver, if it's a release build then we want to load relative to us.
#ifdef _DEBUG
    if ((Raildriver = LoadLibraryA("D:\\Steam\\steamapps\\common\\RailWorks\\plugins\\RailDriver64.dll")) == NULL) { //Try loading the DLL and error out if it failed

        puts("Error loading Raildriver64.dll");
        return false;
    }
#else
    if ((Raildriver = LoadLibraryA("RailDriver64.dll")) == NULL) { //Try loading the DLL and error out if it failed

        puts("Error loading Raildriver64.dll");
        return false;
    }
#endif

    puts("Succesfully loaded Raildriver64.dll");

    //Bind functions and check they all bound correctly, with error handling

    SetRailDriverConnected =    (SetRailDriverConnectedType)GetProcAddress(Raildriver, "SetRailDriverConnected");
    GetControllerList =         (GetControllerListType)GetProcAddress(Raildriver, "GetControllerList");
    SetControllerValue =        (SetControllerValueType)GetProcAddress(Raildriver, "SetControllerValue");
    GetCurrentControllerValue =        (GetCurrentControllerValueType)GetProcAddress(Raildriver, "GetCurrentControllerValue");
    GetLocoName =               (GetLocoNameType)GetProcAddress(Raildriver, "GetLocoName");
    SetRailSimValue =           (SetControllerValueType)GetProcAddress(Raildriver, "SetRailSimValue");
    GetRailSimValue =           (GetCurrentControllerValueType)GetProcAddress(Raildriver, "GetRailSimValue");
    GetControllerValue =        (GetControllerValueType)GetProcAddress(Raildriver, "GetControllerValue");

    if ( (SetRailDriverConnected == NULL) || (GetControllerList == NULL) || (SetControllerValue == NULL) || (GetCurrentControllerValue == NULL)
        || (GetLocoName == NULL) || (SetRailSimValue == NULL) || (GetRailSimValue == NULL) || (GetControllerValue == NULL)) {

        puts("Error acquiring function pointers\n");
        return false;
    }

    puts("Bound functions from Raildriver64.dll succesfully");
    SetRailDriverConnected(true);
    Sleep(500);
    return true; //If alls well return true.
}

bool GrabControlNames() { //This function gets the controller list from the Raildriver DLL and sorts them into a vector

    ClearArray(); //Clear the vector, this is here for when Update terminal function gets called since if our old list was still in the vector the new one would get appended on the end.
    SetRailDriverConnected(true);

    //Bits needed for some upcoming stuff
    int i = 0;
    char* ControlList = GetControllerList(); //Start by getting a pointer to the Controller List from the DLL 

    //Error Handling
    if (ControlList == NULL) { //Test if we got a null pointer

        puts("Error Raildriver returned a NULL pointer");
        return false;
    }

    if (!strlen(ControlList)) { //Test we actually got a non empty string returned

        puts("Error Raildriver returned pointer to Empty String, Retrying...");

        for (char tries = 0; tries < 11; tries++) { //Loop to retry getting a string

            SetRailDriverConnected(true);
            char* ControlList = GetControllerList();

            if (strlen(ControlList) != 0) { break; } //Break if we get a usable string

            if (tries == 10) { puts("Error Raildriver failed to return a non-empty string after 10 attempts"); return false; } //If after 10 attempts we have no string, print an error and return
        }

        puts("After retrying, Raildriver returned a string!");
    }

    if (ControlList == NULL) { puts("Error getting control list"); return false; }

    cout << "Char* Length was " << strlen(ControlList) << "\n";

    char* str = strdup(ControlList); //Duplicate that into our memory for manipulation.

    //Set up the token and we have to get the first one and store it.
    char* token;
    char* working = str;
    token = strdup("cheese");

    while (token != NULL) {
        CVArraySize = i;
     
        //Increment through the array storing the CVs
        token = strtok(working, delim);
        CVArray[i] = strdup(token); //push into array
        //if (CVArray[i] != NULL) { cout << "CV = " << string(token) << endl;  }
        i++;
        working = NULL;
    }

    ListLength = i; //Don't know why I store this, I don't need it in here tbh
    puts("Controls grabbed and setup");
    return true;
}

bool readstr(void) { // This reads the user input on the terminal. I'm using cin because when I used scanf you could up arrow through all the function calls before it and some would cause crashes so I used cin.

    char input[101] = { NULL };
    
    if (scanf(" %100s", &input)) {

        int i = 0;
        char* token = NULL;
        char* working = input;

        while (i != commandsize) {

            //Increment through the array storing the CVs            
            token = strtok(working, ":");
            command[i] = strdup(token);
            //printf("%s\n", token );
            working = NULL;
            i++;
        }


        return true;
    }

    puts("Something went wrong taking input!");
    return false;
}

void readCommand(char* input)
{
    int i = 0;
    char* token = NULL;
    char* working = input;
    char* cutEnd = strstr(input, COMM_SYNTAX_TERM_MSG);
    if (cutEnd != NULL)
        cutEnd[0] = '\0';

    while (i != commandsize) {

        //Increment through the array storing the CVs            
        token = strtok(working, ":");
        command[i] = strdup(token);
        //printf("%s\n", token );
        working = NULL;
        i++;
    }
    return;
}

int getIndex( char* K){  //This it also from stack overflow IIRC and I also don't fully understand it.
    
    for (int i = 0; i != (CVArraySize); i++) {

        if (strcmp(K, CVArray[i]) == 0) {

            cout << "Index for " << CVArray[i] << " was : " << i << endl;
            return i;
        }
    }

    cout << "No Index Found For CV Name : " << K << endl;
    return -1;
}

void ClearArray() { //Free all the memory in the array and replace with a NULL.

    for (int i = 0; i != 512; i++) {

        free(CVArray[i]);
        CVArray[i] = strdup(NULL);
        //cout << "freeing memory in command[" << i << "]\n";
    }

    return;
}

float CSToF(char* str) {

    float ret = (float) atof(str);

    if (ret != 0) {
        return ret;
    }

    if (strcmp(str, "0") == 0) {
        return 0;
    }

    if (strcmp(str, "0.00") == 0) {
        return 0;
    }
    if (strcmp(str, "0.0") == 0) {
        return 0;
    }

    return -32766;
}

bool serialiseControlNames() { //Test function for server, to grab the CV names and serialise it with the current value, min and max.

    
    ZeroMemory(serialised, sizeof(serialised));
    SetRailDriverConnected(true);

    //Bits needed for some upcoming stuff
    int i = 0;
    char* ControlList = GetControllerList(); //Start by getting a pointer to the Controller List from the DLL 

    //Error Handling
    if (ControlList == NULL) { //Test if we got a null pointer

        puts("Error Raildriver returned a NULL pointer");
        return false;
    }

    if (!strlen(ControlList)) { //Test we actually got a non empty string returned

        puts("Error Raildriver returned pointer to Empty String, Retrying...");

        for (char tries = 0; tries < 11; tries++) { //Loop to retry getting a string

            SetRailDriverConnected(true);
            char* ControlList = GetControllerList();

            if (strlen(ControlList) != 0) { break; } //Break if we get a usable string

            if (tries == 10) { puts("Error Raildriver failed to return a non-empty string after 10 attempts"); return false; } //If after 10 attempts we have no string, print an error and return
        }

        puts("After retrying, Raildriver returned a string!");
    }

    if (ControlList == NULL) { puts("Error getting control list"); return false; }

    printf("Char* Length was %d\n", strlen(ControlList));

    char* str = strdup(ControlList); //Duplicate that into our memory for manipulation.

    //Set up the token and we have to get the first one and store it.
    char* token;
    char* working = str;
    token = strdup("cheese");
    strcat(serialised, COMM_SYNTAX_CVLIST);
    while (1) {

        //Increment through the array storing the CVs
        token = strtok(working, delim);
        if (token == NULL) break;
        SetRailDriverConnected(true);
        float curr, min, max;
        curr = GetControllerValue(i, 0);
        min = GetControllerValue(i, 1);
        max = GetControllerValue(i, 2);

        sprintf(serialised, "%s%s::%.2f::%.2f::%.2f??", serialised, token, curr, min, max);
        i++;
        working = NULL;
    }
    strcat(serialised, COMM_SYNTAX_TERM_MSG);
    ListLength = i; //Don't know why I store this, I don't need it in here tbh
    printf("\n\n\nSerialised Control List : \n\n%s\n\n", serialised);
    puts("Controls grabbed and setup");
    send(clientSocket, serialised, strlen(serialised)+1, 0);
    return true;
}

bool runCommand(char* cmd)
{
    //Terminal functions

    if (strcmp(command[0], "Update") == 0) { //Check if Update exists at the start of what was typed, if so then we're doing the Update "function"

        if (!Initialise()) { //Run update again with error handling

            puts("Error performing Update");
            return false;
        }

        puts("Update completed, Initialise was run again");
        return true;
    }    

    else if (strstr(command[0], COMM_SYNTAX_REQ_LOCONAME) != NULL) { //Check if LocoName exists at the start of what was typed, if so then we're doing the LocoName "function"

        /*
        SetRailDriverConnected(true);
        cout << "Current Loco Name is : " << GetLocoName() << endl; //Print loco name
        */
        getAndSendLocoName();
        return true;
    }    

    else if (strcmp(command[0], COMM_SYNTAX_SETCV) == 0) { //Check if Set exists at the start of what was typed, if so then we're doing the Set "function"

        puts("Set Command called");

        if ((!command[1]) || (!command[2])) { //Check that we have the correct number of arguments, this avoids reading off the end of the vector
            puts("Too few arguments, skipping");
            return false;
        }

        
        int CVIndex = getIndex(command[1]);

        float CVValue = CSToF(command[2]); //create a float


        SetRailDriverConnected(true); //we need to set the bool on this

        SetControllerValue(CVIndex, CVValue); //Run the function
        cout << "Set control value " << command[1] << " to " << command[1] << endl; //Print shit.
        return true;
        
    }    

    else if (strcmp(command[0], "Get") == 0) { //Check if Get exists at the start of what was typed, if so then we're doing the Get "function"

        puts("Get Command called");

        if ((!command[1])) {//Check that we have the correct number of arguments, this avoids reading off the end of the vector
            puts("Too few arguments, skipping");
            return false;
        }

        int cvIndex = getIndex(command[1]); 

        SetRailDriverConnected(true); //we need to set the bool on this

        float fncret = 0;

        fncret = GetCurrentControllerValue(cvIndex);
        cout << "Get control value " << command[1] << " was " << fncret << endl; //Print shit out
        return true;
    }       

    else if (strstr(command[0], COMM_SYNTAX_REQ_CVLIST) != NULL) {

        serialiseControlNames();
        return true;
    }

    return false;
}

int printIPAddress()
{
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
        //printf("Error %s when getting local host name.\n", WSAGetLastError());
        return 1;
    }
    //printf("Host name is %s\n", ac);

    struct hostent* phe = gethostbyname(ac);
    if (phe == 0) {
        puts("Yow! Bad host lookup.");
        return 1;
    }

    for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
        struct in_addr addr;
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
        printf("Address %d : %s\n", i, inet_ntoa(addr));
    }

    return 0;
}

//Function that grabs the loco name from the RD DLL and sends it
void getAndSendLocoName()
{
    SetRailDriverConnected(true);
    //Grab name, handle not getting one
    char* locoNamePtr = GetLocoName();
    if (locoNamePtr == NULL)
        return;
    printf("Got loco name - %s\n", locoNamePtr);
    //Assign some memory and check it got assigned
    char* temp = (char*)calloc(strlen(locoNamePtr) + strlen(COMM_SYNTAX_TERM_MSG) + 1, sizeof(char));
    
    if (temp == NULL)
        return;
    //Assemble string, send and then free memory
    strcpy(temp, locoNamePtr);
    strcat(temp, COMM_SYNTAX_TERM_MSG);
    send(clientSocket, temp, strlen(temp) + 1, 0);
    printf("Sent string \"%s\" via socket\n", temp);
    free(temp);
    return;
}