//Includes
//-----------------------
#include <citro2d.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "ui-defines.h"
#include "tech-defines.h"
#include "structs.h"


//Text Rendering Stuff
//-----------------------
C2D_TextBuf gStaticTextBuffer;
C2D_Text gStaticText[S_STRING_COUNT];
C2D_TextBuf gDynamicTextBuffer;

//Sprite variables
//-----------------------
static C2D_SpriteSheet topScrSpriteSheet;
static C2D_SpriteSheet botScrSpriteSheet;
static UISprite topScrUISprites[TOP_UISPRITECOUNT];
static UISprite botScrUISprites[BOT_UISPRITECOUNT];

//Globals
static bool currentControlState = false; 						//Are we using controls for the UI or for controlling TS
static bool configCVIsAxis		= false; 						//Are we configuring an Axis, if not it's a pushbutton
static bool networkingConnected = false; 						//Self-explanatory
static float spriteX = 0.5, spriteY = 0.5; 						//Deprecate
char targetIPAddress[IP_ADDRESS_MAX_SIZE] = "192.168.1.226"; 	//Default IP Address
char buf[16386];												//Buffer for reading in from socket
axisControl cpadVertAxis, cpadHoriAxis, dpadVertAxis, dpadHoriAxis;
pushbuttonControl lPushbutton, rPushbutton, aPushbutton, 
					bPushbutton, xPushbutton, yPushbutton;
controlValue cvList[CV_LIST_CV_COUNT];							//Array of control values
axisControl* currentAxisControl;								//Pointer to the Axis we're currently configuring
pushbuttonControl* currentPBControl;							
int sock;														//int for the socket
C3D_RenderTarget* bot;											//Pointer to the bottom screen

//Function prototypes
//-----------------------
static void initTextRender();
static void createTopScreenUISprites();
static void createBotScreenUISprites();
static void strcatToConnConsole(char* source);
static void dynamicTextCreate(C2D_Text *text, char* string);
static void drawConnConsole( bool redrawing);
static void populateCVList(char* list);
static void sendMessageViaSocket(char* inString);
static void recvCompleteMessageFromSockTobuf();
void fetchAndHandleControlList();
void fetchAndHandleLocoName(char* locoNamePointer);
void resetControlValueStruct(controlValue* cv);
void setControlValueStruct(controlValue* cv, char* setCVName, float setCurr, float setMin, float setMax);
void resetAxisControlStruct(axisControl* axis);
void setAxisControlStruct(axisControl* axis, controlValue* cv);
void resetPBControlStruct(pushbuttonControl* axis);
void setPBControlStruct(pushbuttonControl* axis, controlValue* cv);
void handleSettingCVForCurrentStruct();
float returnCVCentre(controlValue* cv);



int main(int argc, char* argv[])
{

	for(int i = 0; i > CV_LIST_CV_COUNT; i++)
		resetControlValueStruct(&cvList[i]);
	

	setControlValueStruct(&cvList[0], "CV List", 0,0,0);
	setControlValueStruct(&cvList[1], "Not Intialised", 0,0,0);
	setControlValueStruct(&cvList[2], "Please connect to server...", 0,0,0);
	setControlValueStruct(&cvList[3], "Known New Entry", 0,0,0);
	setControlValueStruct(&cvList[4], "", 0,0,0);
	setControlValueStruct(&cvList[5], "", 0,0,0);
	setControlValueStruct(&cvList[6], "", 0,0,0);

	resetAxisControlStruct(&cpadVertAxis);
	resetAxisControlStruct(&cpadHoriAxis);
	resetAxisControlStruct(&dpadVertAxis);
	resetAxisControlStruct(&dpadHoriAxis);
	strcpy(cpadVertAxis.axisName , "CPad Vertical Axis");
	strcpy(cpadHoriAxis.axisName , "CPad Horizontal Axis");
	strcpy(dpadVertAxis.axisName , "DPad Vertical Axis");
	strcpy(dpadHoriAxis.axisName , "DPad Horizontal Axis");

	resetPBControlStruct(&lPushbutton);
	resetPBControlStruct(&rPushbutton);
	resetPBControlStruct(&aPushbutton);
	resetPBControlStruct(&bPushbutton);
	resetPBControlStruct(&xPushbutton);
	resetPBControlStruct(&yPushbutton);
	strcpy(lPushbutton.pushbuttonName , "L Button");
	strcpy(rPushbutton.pushbuttonName , "R Button");
	strcpy(aPushbutton.pushbuttonName , "A Button");
	strcpy(bPushbutton.pushbuttonName , "B Button");
	strcpy(xPushbutton.pushbuttonName , "X Button");
	strcpy(yPushbutton.pushbuttonName , "Y Button");


	currentAxisControl = &cpadVertAxis;
	configCVIsAxis = true;

	//Init the TestCVList
	/*
	strcpy(testCVList[0],"CV List");
	strcpy(testCVList[1],"Not Intialised");
	strcpy(testCVList[2],"Please connect to server...");
	strcpy(testCVList[3],"");
	strcpy(testCVList[4],"");
	strcpy(testCVList[5],"");
	strcpy(testCVList[6],"");
	strcpy(testCVList[7],"");
	strcpy(testCVList[8],"");
	strcpy(testCVList[9],"");
	strcpy(testCVList[10],"");
	strcpy(testCVList[11],"");
	strcpy(testCVList[12],"");
	strcpy(testCVList[13],"");
	strcpy(testCVList[14],"");
	strcpy(testCVList[15],"");
	*/

	//Networking init
	int ret;
	

	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
		printf("memalign: failed to allocate\n");
		return 1;
	}
	strcatToConnConsole("memalign: allocated!\n");

	// Now intialise soc:u service
	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
		printf("socInit: 0x%08X\n", (unsigned int)ret);
		return 1;
	}
	strcatToConnConsole("soc:u service init!\n");

	struct sockaddr_in addr;
	/*int sock,*/ int res;

	// Init libs
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	// Create screens
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	// Load graphics and error check
	topScrSpriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/topsprites.t3x");
	if (!topScrSpriteSheet) svcBreak(USERBREAK_PANIC);

	botScrSpriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/botsprites.t3x");
	if (!botScrSpriteSheet) svcBreak(USERBREAK_PANIC);

	// Initialize sprites and create static text objects
	createTopScreenUISprites();	
	createBotScreenUISprites();
	initTextRender();

	//printf("\x1b[8;1HPress Up to increment sprites");
	//printf("\x1b[9;1HPress Down to decrement sprites");

	//Create some variables
	int botScreenState = BOT_TAB_STATE_CONNECTION;
	int wifiState = 0;
	touchPosition oldTouch;
	char locoNamePointer[70] = "";
	
	struct touchButtonsPressed touchButtons;
	


	// Main loop
	while (aptMainLoop())
	{
		//Make the buf string ""
		buf[0] = '\0';

		//Get info from the system that we're interested in
		hidScanInput();
		wifiState = osGetWifiStrength();

		//This runs in an update loop so I assume false for everything by memsetting the bitflag variable
		memset(&touchButtons, 0, sizeof(touchButtons)); 

		//Grab the touch screen touch pos and add a slight adjustment to it
		touchPosition touch;
		hidTouchRead(&touch);
		touch.px -= TOUCH_SCREEN_ADJ_X;
		touch.py -= TOUCH_SCREEN_ADJ_Y;
		
		// Respond to user input
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		u32 kUp = hidKeysUp();

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if ((kDown & KEY_SELECT))
		{
			//Flips whether the controls are controlling TS or the app itself
			currentControlState = !currentControlState;			
		}
		/*
		if (kDown & KEY_X)
		{
			if (networkingConnected)
			{				
				//fetchAndHandleControlList();
				
				sprintf(buf, "%s:%s:%f", COMM_SYNTAX_SETCV, "VirtualAWSReset", 1.0);
				sendMessageViaSocket(buf);
				buf[0] = '\0';
				//sendMessageViaSocket("SET_CV//:VirtualAWSReset:1.0//END//");
			}			
			
		}
		if (kDown & KEY_Y)
		{
			if (networkingConnected)
			{						
				//char tempBuf[128] = {'\0'};
				sprintf(buf, "%s:%s:%f", COMM_SYNTAX_SETCV, "VirtualAWSReset", 0.0);
				sendMessageViaSocket(buf);
				buf[0] = '\0';
			}			
		}
		if (kDown & KEY_A)
		{
			//fetchAndHandleLocoName(locoNamePointer);
			currentAxisControl->hasValidCV = true;
			currentAxisControl->control = &cvList[0];
		}
		if (kDown & KEY_B)
		{
			strcatToConnConsole("KEY_B Pressed\n");
			char Abuf[] = "TestMessage2";
			res = send (sock, Abuf, strlen(Abuf), 0);
			if (res < 0) {
				strcatToConnConsole("Server connection lost\n");
				networkingConnected = false;
				drawConnConsole(true);
				close(sock);
				strcatToConnConsole("Closed socket\n");
			}
		}
		*/
		if ((kDown & KEY_LEFT) && !currentControlState )
		{
			botScreenState = (botScreenState > 0) ? --botScreenState : botScreenState;
			//char temp[10];
			//sprintf(temp, "Var = %d\n", botScreenState);
			//strcatToConnConsole(temp);
		}
		if ((kDown & KEY_RIGHT) && !currentControlState )
		{
			botScreenState = (botScreenState < BOT_TAB_STATE_MAX) ? ++botScreenState : botScreenState;
			//char temp[10];
			//sprintf(temp, "Var = %d\n", botScreenState);
			//strcatToConnConsole(temp);
		}
		
		
		//This handles the control value being move
		if (currentControlState)
		{
			//Handle CPAD Vertical Axis
			//-------------------------
			if (cpadVertAxis.hasValidCV)
			{
				if (cpadVertAxis.sprungToCentre)
				{
					if (kDown & KEY_CPAD_UP)
					{
						cpadVertAxis.control->current = cpadVertAxis.control->max;
					}
					if (kDown & KEY_CPAD_DOWN)
					{
						cpadVertAxis.control->current = cpadVertAxis.control->min;
					}
					if ( ((kUp & KEY_CPAD_DOWN)) || ((kUp & KEY_CPAD_UP)) )
					{
						cpadVertAxis.control->current = returnCVCentre(cpadVertAxis.control);
					}

					//if (abs(cpadVertAxis.control->current - cpadVertAxis.control->currentOld) > MAX_FLOAT_RANGE_EQUAL) 
					if (VALUES_ARENT_WITHIN( cpadVertAxis.control->current, cpadVertAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
					{
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, cpadVertAxis.control->name, cpadVertAxis.control->current);
						sendMessageViaSocket(sendBuffer);	
						cpadVertAxis.control->currentOld = cpadVertAxis.control->current;	
					}

				}				
				else
				{
					if (kHeld & KEY_CPAD_UP)
					{
						cpadVertAxis.control->current += (cpadVertAxis.control->current < cpadVertAxis.control->max) ? SPRING_TO_CENTRE_SENSIT : 0;
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "current is now %.2f\n", cpadVertAxis.control->current);
						strcatToConnConsole(sendBuffer);
					}
					if (kHeld & KEY_CPAD_DOWN)
					{
						cpadVertAxis.control->current -= (cpadVertAxis.control->current > cpadVertAxis.control->min) ? SPRING_TO_CENTRE_SENSIT : 0;
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "current is now %.2f\n", cpadVertAxis.control->current);
						strcatToConnConsole(sendBuffer);
					}

					//if (abs(cpadVertAxis.control->current - cpadVertAxis.control->currentOld) > MAX_FLOAT_RANGE_EQUAL) 
					if (VALUES_ARENT_WITHIN( cpadVertAxis.control->current, cpadVertAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL)) //|| !VALUES_ARENT_WITHIN( cpadVertAxis.control->current, cpadVertAxis.control->min,MAX_FLOAT_RANGE_EQUAL)
					//|| !VALUES_ARENT_WITHIN( cpadVertAxis.control->current, cpadVertAxis.control->max, MAX_FLOAT_RANGE_EQUAL))
					//if (VALUES_ARENT_WITHIN( cpadVertAxis.control->current, cpadVertAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
					{
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, cpadVertAxis.control->name, cpadVertAxis.control->current);
						sendMessageViaSocket(sendBuffer);	
						cpadVertAxis.control->currentOld = cpadVertAxis.control->current;	
					}
				}
					
			}	

			//Handle CPAD Horizontal Axis
			//-------------------------
			if (cpadHoriAxis.hasValidCV)
			{
				if (cpadHoriAxis.sprungToCentre)
				{
					if (kDown & KEY_CPAD_RIGHT)
					{
						cpadHoriAxis.control->current = cpadHoriAxis.control->max;
					}
					if (kDown & KEY_CPAD_LEFT)
					{
						cpadHoriAxis.control->current = cpadHoriAxis.control->min;
					}
					if ( ((kUp & KEY_CPAD_LEFT)) || ((kUp & KEY_CPAD_RIGHT)) )
					{
						cpadHoriAxis.control->current = returnCVCentre(cpadHoriAxis.control);
					}

					//if (abs(cpadHoriAxis.control->current - cpadHoriAxis.control->currentOld) > MAX_FLOAT_RANGE_EQUAL) 
					if (VALUES_ARENT_WITHIN( cpadHoriAxis.control->current, cpadHoriAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
					{
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, cpadHoriAxis.control->name, cpadHoriAxis.control->current);
						sendMessageViaSocket(sendBuffer);	
						cpadHoriAxis.control->currentOld = cpadHoriAxis.control->current;	
					}

				}				
				else
				{
					if (kHeld & KEY_CPAD_RIGHT)
					{
						cpadHoriAxis.control->current += (cpadHoriAxis.control->current < cpadHoriAxis.control->max) ? SPRING_TO_CENTRE_SENSIT : 0;
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "current is now %.2f\n", cpadHoriAxis.control->current);
						strcatToConnConsole(sendBuffer);
					}
					if (kHeld & KEY_CPAD_LEFT)
					{
						cpadHoriAxis.control->current -= (cpadHoriAxis.control->current > cpadHoriAxis.control->min) ? SPRING_TO_CENTRE_SENSIT : 0;
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "current is now %.2f\n", cpadHoriAxis.control->current);
						strcatToConnConsole(sendBuffer);
					}

					//if (abs(cpadHoriAxis.control->current - cpadHoriAxis.control->currentOld) > MAX_FLOAT_RANGE_EQUAL) 
					if (VALUES_ARENT_WITHIN( cpadHoriAxis.control->current, cpadHoriAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL)) //|| !VALUES_ARENT_WITHIN( cpadHoriAxis.control->current, cpadHoriAxis.control->min,MAX_FLOAT_RANGE_EQUAL)
					//|| !VALUES_ARENT_WITHIN( cpadHoriAxis.control->current, cpadHoriAxis.control->max, MAX_FLOAT_RANGE_EQUAL))
					//if (VALUES_ARENT_WITHIN( cpadHoriAxis.control->current, cpadHoriAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
					{
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, cpadHoriAxis.control->name, cpadHoriAxis.control->current);
						sendMessageViaSocket(sendBuffer);	
						cpadHoriAxis.control->currentOld = cpadHoriAxis.control->current;	
					}
				}
					
			}	

			//Handle dpad Vertical Axis
			//-------------------------
			if (dpadVertAxis.hasValidCV)
			{
				if (dpadVertAxis.sprungToCentre)
				{
					if (kDown & KEY_DUP)
					{
						dpadVertAxis.control->current = dpadVertAxis.control->max;
					}
					if (kDown & KEY_DDOWN)
					{
						dpadVertAxis.control->current = dpadVertAxis.control->min;
					}
					if ( ((kUp & KEY_DDOWN)) || ((kUp & KEY_DUP)) )
					{
						dpadVertAxis.control->current = returnCVCentre(dpadVertAxis.control);
					}

					//if (abs(dpadVertAxis.control->current - dpadVertAxis.control->currentOld) > MAX_FLOAT_RANGE_EQUAL) 
					if (VALUES_ARENT_WITHIN( dpadVertAxis.control->current, dpadVertAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
					{
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, dpadVertAxis.control->name, dpadVertAxis.control->current);
						sendMessageViaSocket(sendBuffer);	
						dpadVertAxis.control->currentOld = dpadVertAxis.control->current;	
					}

				}				
				else
				{
					if (kHeld & KEY_DUP)
					{
						dpadVertAxis.control->current += (dpadVertAxis.control->current < dpadVertAxis.control->max) ? SPRING_TO_CENTRE_SENSIT : 0;
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "current is now %.2f\n", dpadVertAxis.control->current);
						strcatToConnConsole(sendBuffer);
					}
					if (kHeld & KEY_DDOWN)
					{
						dpadVertAxis.control->current -= (dpadVertAxis.control->current > dpadVertAxis.control->min) ? SPRING_TO_CENTRE_SENSIT : 0;
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "current is now %.2f\n", dpadVertAxis.control->current);
						strcatToConnConsole(sendBuffer);
					}

					//if (abs(dpadVertAxis.control->current - dpadVertAxis.control->currentOld) > MAX_FLOAT_RANGE_EQUAL) 
					if (VALUES_ARENT_WITHIN( dpadVertAxis.control->current, dpadVertAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL)) //|| !VALUES_ARENT_WITHIN( dpadVertAxis.control->current, dpadVertAxis.control->min,MAX_FLOAT_RANGE_EQUAL)
					//|| !VALUES_ARENT_WITHIN( dpadVertAxis.control->current, dpadVertAxis.control->max, MAX_FLOAT_RANGE_EQUAL))
					//if (VALUES_ARENT_WITHIN( dpadVertAxis.control->current, dpadVertAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
					{
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, dpadVertAxis.control->name, dpadVertAxis.control->current);
						sendMessageViaSocket(sendBuffer);	
						dpadVertAxis.control->currentOld = dpadVertAxis.control->current;	
					}
				}
					
			}	

			//Handle dpad Horizontal Axis
			//-------------------------
			if (dpadHoriAxis.hasValidCV)
			{
				if (dpadHoriAxis.sprungToCentre)
				{
					if (kDown & KEY_DRIGHT)
					{
						dpadHoriAxis.control->current = dpadHoriAxis.control->max;
					}
					if (kDown & KEY_DLEFT)
					{
						dpadHoriAxis.control->current = dpadHoriAxis.control->min;
					}
					if ( ((kUp & KEY_DLEFT)) || ((kUp & KEY_DRIGHT)) )
					{
						dpadHoriAxis.control->current = returnCVCentre(dpadHoriAxis.control);
					}

					//if (abs(dpadHoriAxis.control->current - dpadHoriAxis.control->currentOld) > MAX_FLOAT_RANGE_EQUAL) 
					if (VALUES_ARENT_WITHIN( dpadHoriAxis.control->current, dpadHoriAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
					{
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, dpadHoriAxis.control->name, dpadHoriAxis.control->current);
						sendMessageViaSocket(sendBuffer);	
						dpadHoriAxis.control->currentOld = dpadHoriAxis.control->current;	
					}

				}				
				else
				{
					if (kHeld & KEY_DRIGHT)
					{
						dpadHoriAxis.control->current += (dpadHoriAxis.control->current < dpadHoriAxis.control->max) ? SPRING_TO_CENTRE_SENSIT : 0;
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "current is now %.2f\n", dpadHoriAxis.control->current);
						strcatToConnConsole(sendBuffer);
					}
					if (kHeld & KEY_DLEFT)
					{
						dpadHoriAxis.control->current -= (dpadHoriAxis.control->current > dpadHoriAxis.control->min) ? SPRING_TO_CENTRE_SENSIT : 0;
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "current is now %.2f\n", dpadHoriAxis.control->current);
						strcatToConnConsole(sendBuffer);
					}

					//if (abs(dpadHoriAxis.control->current - dpadHoriAxis.control->currentOld) > MAX_FLOAT_RANGE_EQUAL) 
					if (VALUES_ARENT_WITHIN( dpadHoriAxis.control->current, dpadHoriAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL)) //|| !VALUES_ARENT_WITHIN( dpadHoriAxis.control->current, dpadHoriAxis.control->min,MAX_FLOAT_RANGE_EQUAL)
					//|| !VALUES_ARENT_WITHIN( dpadHoriAxis.control->current, dpadHoriAxis.control->max, MAX_FLOAT_RANGE_EQUAL))
					//if (VALUES_ARENT_WITHIN( dpadHoriAxis.control->current, dpadHoriAxis.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
					{
						char sendBuffer[200] = {'\0'};
						sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, dpadHoriAxis.control->name, dpadHoriAxis.control->current);
						sendMessageViaSocket(sendBuffer);	
						dpadHoriAxis.control->currentOld = dpadHoriAxis.control->current;	
					}
				}
					
			}	

			//L Pushbutton
			//----------------
			if (lPushbutton.hasValidCV)	
			{
				if (kDown & KEY_L)
				{
					lPushbutton.control->current = (VALUES_ARE_WITHIN(lPushbutton.control->max, lPushbutton.control->current, MAX_FLOAT_RANGE_EQUAL)) ? lPushbutton.control->min : lPushbutton.control->max;
				}
				if (kUp & KEY_L)
				{
					if (!lPushbutton.toggle)
					{
						lPushbutton.control->current = lPushbutton.control->min;
					}
				}

				if (VALUES_ARENT_WITHIN( lPushbutton.control->current, lPushbutton.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
				{
					char sendBuffer[200] = {'\0'};
					sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, lPushbutton.control->name, lPushbutton.control->current);
					sendMessageViaSocket(sendBuffer);	
					lPushbutton.control->currentOld = lPushbutton.control->current;	
				}
			}

			//R Pushbutton
			//----------------
			if (rPushbutton.hasValidCV)	
			{
				if (kDown & KEY_R)
				{
					rPushbutton.control->current = (VALUES_ARE_WITHIN(rPushbutton.control->max, rPushbutton.control->current, MAX_FLOAT_RANGE_EQUAL)) ? rPushbutton.control->min : rPushbutton.control->max;
				}
				if (kUp & KEY_R)
				{
					if (!rPushbutton.toggle)
					{
						rPushbutton.control->current = rPushbutton.control->min;
					}
				}

				if (VALUES_ARENT_WITHIN( rPushbutton.control->current, rPushbutton.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
				{
					char sendBuffer[200] = {'\0'};
					sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, rPushbutton.control->name, rPushbutton.control->current);
					sendMessageViaSocket(sendBuffer);	
					rPushbutton.control->currentOld = rPushbutton.control->current;	
				}
			}

			//A Pushbutton
			//----------------
			if (aPushbutton.hasValidCV)	
			{
				if (kDown & KEY_A)
				{
					aPushbutton.control->current = (VALUES_ARE_WITHIN(aPushbutton.control->max, aPushbutton.control->current, MAX_FLOAT_RANGE_EQUAL)) ? aPushbutton.control->min : aPushbutton.control->max;
				}
				if (kUp & KEY_A)
				{
					if (!aPushbutton.toggle)
					{
						aPushbutton.control->current = aPushbutton.control->min;
					}
				}

				if (VALUES_ARENT_WITHIN( aPushbutton.control->current, aPushbutton.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
				{
					char sendBuffer[200] = {'\0'};
					sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, aPushbutton.control->name, aPushbutton.control->current);
					sendMessageViaSocket(sendBuffer);	
					aPushbutton.control->currentOld = aPushbutton.control->current;	
				}
			}

			//B Pushbutton
			//----------------
			if (bPushbutton.hasValidCV)	
			{
				if (kDown & KEY_B)
				{
					bPushbutton.control->current = (VALUES_ARE_WITHIN(bPushbutton.control->max, bPushbutton.control->current, MAX_FLOAT_RANGE_EQUAL)) ? bPushbutton.control->min : bPushbutton.control->max;
				}
				if (kUp & KEY_B)
				{
					if (!bPushbutton.toggle)
					{
						bPushbutton.control->current = bPushbutton.control->min;
					}
				}

				if (VALUES_ARENT_WITHIN( bPushbutton.control->current, bPushbutton.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
				{
					char sendBuffer[200] = {'\0'};
					sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, bPushbutton.control->name, bPushbutton.control->current);
					sendMessageViaSocket(sendBuffer);	
					bPushbutton.control->currentOld = bPushbutton.control->current;	
				}
			}

			//X Pushbutton
			//----------------
			if (xPushbutton.hasValidCV)	
			{
				if (kDown & KEY_X)
				{
					xPushbutton.control->current = (VALUES_ARE_WITHIN(xPushbutton.control->max, xPushbutton.control->current, MAX_FLOAT_RANGE_EQUAL)) ? xPushbutton.control->min : xPushbutton.control->max;
				}
				if (kUp & KEY_X)
				{
					if (!xPushbutton.toggle)
					{
						xPushbutton.control->current = xPushbutton.control->min;
					}
				}

				if (VALUES_ARENT_WITHIN( xPushbutton.control->current, xPushbutton.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
				{
					char sendBuffer[200] = {'\0'};
					sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, xPushbutton.control->name, xPushbutton.control->current);
					sendMessageViaSocket(sendBuffer);	
					xPushbutton.control->currentOld = xPushbutton.control->current;	
				}
			}

			//Y Pushbutton
			//----------------
			if (yPushbutton.hasValidCV)	
			{
				if (kDown & KEY_Y)
				{
					yPushbutton.control->current = (VALUES_ARE_WITHIN(yPushbutton.control->max, yPushbutton.control->current, MAX_FLOAT_RANGE_EQUAL)) ? yPushbutton.control->min : yPushbutton.control->max;
				}
				if (kUp & KEY_Y)
				{
					if (!yPushbutton.toggle)
					{
						yPushbutton.control->current = yPushbutton.control->min;
					}
				}

				if (VALUES_ARENT_WITHIN( yPushbutton.control->current, yPushbutton.control->currentOld, MAX_FLOAT_RANGE_EQUAL))
				{
					char sendBuffer[200] = {'\0'};
					sprintf(sendBuffer, "%s:%s:%.2f", COMM_SYNTAX_SETCV, yPushbutton.control->name, yPushbutton.control->current);
					sendMessageViaSocket(sendBuffer);	
					yPushbutton.control->currentOld = yPushbutton.control->current;	
				}
			}
		}


		
		if ((kHeld & KEY_UP) && spriteY < 1)
		{
			//Scrolling the CV menu with DPAD or CPAD
			if (botScreenState == BOT_TAB_STATE_CONTROLVALUES && !currentControlState)
			{
				if (canUpScroll)
				{
					int temp = (!(kHeld & KEY_L)) ? 2 : 5;
					temp += (!(kHeld & KEY_R)) ? 0 : temp;
					CV_SCR_LIST_SPIRTE_Y -= temp ;
				}
				
				if (!canDnScroll)
					canDnScroll = true;
			}
		}
		if ((kHeld & KEY_DOWN) && spriteY > 0)
		{
			//Scrolling the CV menu with DPAD or CPAD
			if (botScreenState == BOT_TAB_STATE_CONTROLVALUES && !currentControlState)
			{
				if (canDnScroll)
				{
					int temp = (!(kHeld & KEY_L)) ? 2 : 5;
					temp += (!(kHeld & KEY_R)) ? 0 : temp;
					CV_SCR_LIST_SPIRTE_Y += temp ;
				}
				if (!canUpScroll)
					canUpScroll = true;
			}
		}

		//Handle touch screen
		if (kHeld & KEY_TOUCH ) //If the touch screen is pressed 
		{
						
			if (botScreenState == BOT_TAB_STATE_CONNECTION)
			{
				//these set flags that are used to set animations for the buttons on this screen
				if ((touch.px > CON_SCR_BUT_CONNECT_TOUCH_X) && (touch.px < CON_SCR_BUT_CONNECT_TOUCH_X + CON_SCR_BUT_CONNECT_SIZE_X) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CON_SCR_BUT_CONNECT_TOUCH_Y) && (touch.py < CON_SCR_BUT_CONNECT_TOUCH_Y + CON_SCR_BUT_CONNECT_SIZE_Y))			//are within the hitbox that I defined for the button
				{
					touchButtons.connScrConnect = true;				
									
				}

				if ((touch.px > CON_SCR_BUT_DISCONNECT_TOUCH_X) && (touch.px < CON_SCR_BUT_DISCONNECT_TOUCH_X + CON_SCR_BUT_DISCONNECT_SIZE_X) &&
				(touch.py > CON_SCR_BUT_DISCONNECT_TOUCH_Y) && (touch.py < CON_SCR_BUT_DISCONNECT_TOUCH_Y + CON_SCR_BUT_DISCONNECT_SIZE_Y))
				{
					touchButtons.connScrDisconnect = true;					
				}

				if ((touch.px > CON_SCR_BUT_IPADDR_TOUCH_X) && (touch.px < CON_SCR_BUT_IPADDR_TOUCH_X + BOT_TAB_CONTROLS_SIZE_X) &&
				(touch.py > CON_SCR_BUT_IPADDR_TOUCH_Y) && (touch.py < CON_SCR_BUT_IPADDR_TOUCH_Y + BOT_TAB_CONTROLS_SIZE_Y))
				{
					touchButtons.connScrIPAddr = true;					
				}
			}

			if (botScreenState == BOT_TAB_STATE_CONTROLVALUES)
			{
				//Touch scrolling for the CV list (KNOWN BUGGY)
				if (touch.py > CV_SCR_TOUCH_Y_MIN)
				{
					//Calculate Delta then clamp it.
					int delta = touch.py - oldTouch.py;
					delta = (delta > 2) ? 2 : delta;
					delta = (delta < -2) ? -2 : delta;
					delta *= 2;

					//Debug info
					{
						char temp[20];
						sprintf(temp, "delta was %d\n", delta);
						strcatToConnConsole(temp);
					}
					
					//Apply the delta
					if (delta > 0)
					{
						if (canDnScroll)
						{							
							CV_SCR_LIST_SPIRTE_Y += delta ;
						}
					}
					else
					{
						if (canUpScroll)
						{							
							CV_SCR_LIST_SPIRTE_Y += delta ;
						}
					}	
					
				}
			}

			if (botScreenState == BOT_TAB_STATE_CONFIG)
			{
				//these set flags that are used to set animations for the buttons on this screen
				if ((touch.px > CFG_SCR_BUTT_CPAD_VERT_TOUCH_X) && (touch.px < CFG_SCR_BUTT_CPAD_VERT_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_CPAD_VERT_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_CPAD_VERT_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrCPADVert = true;				
									
				}	

				if ((touch.px > CFG_SCR_BUTT_CPAD_HORI_TOUCH_X) && (touch.px < CFG_SCR_BUTT_CPAD_HORI_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_CPAD_HORI_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_CPAD_HORI_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrCPADHori = true;				
									
				}	

				if ((touch.px > CFG_SCR_BUTT_DPAD_VERT_TOUCH_X) && (touch.px < CFG_SCR_BUTT_DPAD_VERT_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_DPAD_VERT_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_DPAD_VERT_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrDPADVert = true;				
									
				}

				if ((touch.px > CFG_SCR_BUTT_DPAD_HORI_TOUCH_X) && (touch.px < CFG_SCR_BUTT_DPAD_HORI_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_DPAD_HORI_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_DPAD_HORI_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrDPADHori = true;				
									
				}

				if ((touch.px > CFG_SCR_BUTT_L_TOUCH_X) && (touch.px < CFG_SCR_BUTT_L_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_L_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_L_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrL = true;				
									
				}

				if ((touch.px > CFG_SCR_BUTT_A_TOUCH_X) && (touch.px < CFG_SCR_BUTT_A_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_A_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_A_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrA = true;				
									
				}

				if ((touch.px > CFG_SCR_BUTT_B_TOUCH_X) && (touch.px < CFG_SCR_BUTT_B_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_B_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_B_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrB = true;				
									
				}

				if ((touch.px > CFG_SCR_BUTT_X_TOUCH_X) && (touch.px < CFG_SCR_BUTT_X_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_X_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_X_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrX = true;				
									
				}

				if ((touch.px > CFG_SCR_BUTT_Y_TOUCH_X) && (touch.px < CFG_SCR_BUTT_Y_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_Y_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_Y_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrY = true;				
									
				}

				if ((touch.px > CFG_SCR_BUTT_R_TOUCH_X) && (touch.px < CFG_SCR_BUTT_R_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
				(touch.py > CFG_SCR_BUTT_R_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_R_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
				{
					touchButtons.cfgScrR = true;				
									
				}			
			}
		}

		if (kDown & KEY_TOUCH ) //Handle key down touch screen events
		{
			if ((touch.px > BOT_TAB_CONN_TOUCH_X) && (touch.px < BOT_TAB_CONN_TOUCH_X + BOT_TAB_CONN_SIZE_X) &&
				(touch.py > BOT_TAB_CONN_TOUCH_Y) && (touch.py < BOT_TAB_CONN_TOUCH_Y + BOT_TAB_CONN_SIZE_Y))
				{
					botScreenState = BOT_TAB_STATE_CONNECTION;					
				}

			if ((touch.px > BOT_TAB_CONTROLS_TOUCH_X) && (touch.px < BOT_TAB_CONTROLS_TOUCH_X + BOT_TAB_CONTROLS_SIZE_X) &&
				(touch.py > BOT_TAB_CONTROLS_TOUCH_Y) && (touch.py < BOT_TAB_CONTROLS_TOUCH_Y + BOT_TAB_CONTROLS_SIZE_Y))
				{
					botScreenState = BOT_TAB_STATE_CONTROLVALUES;				
				}

			if ((touch.px > BOT_TAB_CONFIG_TOUCH_X) && (touch.px < BOT_TAB_CONFIG_TOUCH_X + BOT_TAB_CONFIG_SIZE_X) &&
				(touch.py > BOT_TAB_CONFIG_TOUCH_Y) && (touch.py < BOT_TAB_CONFIG_TOUCH_Y + BOT_TAB_CONFIG_SIZE_Y))
				{
					botScreenState = BOT_TAB_STATE_CONFIG;				
				}

			switch(botScreenState) //State Machine to handle different screens showing
			{
				case BOT_TAB_STATE_CONNECTION:

						//these tests look scary but basically they just test if we clicked within our hitbox
					if ((touch.px > CON_SCR_BUT_CONNECT_TOUCH_X) && (touch.px < CON_SCR_BUT_CONNECT_TOUCH_X + CON_SCR_BUT_CONNECT_SIZE_X) &&
					(touch.py > CON_SCR_BUT_CONNECT_TOUCH_Y) && (touch.py < CON_SCR_BUT_CONNECT_TOUCH_Y + CON_SCR_BUT_CONNECT_SIZE_Y))
					{	
						//Trying to connect if we're connected would cause an issue
						if (!networkingConnected) 
						{
							//Networking connect

							strcatToConnConsole("Connect Pressed\n");
							drawConnConsole(true);
							//Redraw the conn console after so it updates

							networkingConnected = true;
							sock = socket(AF_INET, SOCK_STREAM, 0 );
							if (sock == BSD_ERROR)
							{
								strcatToConnConsole("Error creating socket\n");
								networkingConnected = false;
								break;
							}

							strcatToConnConsole("Created socket!\n");
							drawConnConsole(true);

							addr.sin_port			= htons(PORT);
							addr.sin_family 		= AF_INET;
							addr.sin_addr.s_addr 	= inet_addr(targetIPAddress);

							strcatToConnConsole("Set socket to non-blocking\n");
							strcatToConnConsole("Attempting connection...\n");
							drawConnConsole(true);


							int ret = connect (sock, (struct sockaddr *)&addr, sizeof (addr));
							if (ret != BSD_SUCCESS )
							{
								networkingConnected = false;
								close(sock);
								strcatToConnConsole("Error connecting\n");
								drawConnConsole(true);
								break;
							}
							
							strcatToConnConsole("Connected!\n");
							strcatToConnConsole("Fetching train info..\n");
							drawConnConsole(true);

							//Fetch the stuff we're interested in

							//Okay I have no idea but if you do this the opposite way round it crashes
							fetchAndHandleControlList();
							fetchAndHandleLocoName(locoNamePointer);
							
						}
					}

					if ((touch.px > CON_SCR_BUT_DISCONNECT_TOUCH_X) && (touch.px < CON_SCR_BUT_DISCONNECT_TOUCH_X + CON_SCR_BUT_DISCONNECT_SIZE_X) &&
					(touch.py > CON_SCR_BUT_DISCONNECT_TOUCH_Y) && (touch.py < CON_SCR_BUT_DISCONNECT_TOUCH_Y + CON_SCR_BUT_DISCONNECT_SIZE_Y))
					{
						//Disconnecting if connected
						if (networkingConnected)
						{
							strcatToConnConsole("Disconnect Pressed\n");							
							networkingConnected = false;							
							close(sock);
							strcatToConnConsole("Closed socket\n");
							drawConnConsole(true);
						}
					}
					if ((touch.px > CON_SCR_BUT_IPADDR_TOUCH_X) && (touch.px < CON_SCR_BUT_IPADDR_TOUCH_X + BOT_TAB_CONTROLS_SIZE_X) &&
					(touch.py > CON_SCR_BUT_IPADDR_TOUCH_Y) && (touch.py < CON_SCR_BUT_IPADDR_TOUCH_Y + BOT_TAB_CONTROLS_SIZE_Y))
					{
						//Inputting an IP Address with the Software Keyboard
						if (!networkingConnected)
						{
							//Create a software keyboard struct and initialise it
							SwkbdState swkbd;
							swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, IP_ADDRESS_MAX_SIZE);
							//Various set features
							swkbdSetValidation(&swkbd, SWKBD_ANYTHING, 0, 0);
							swkbdSetFeatures(&swkbd, SWKBD_FIXED_WIDTH);
							//Set what type of keyboard, we're using the numpad
							swkbdSetNumpadKeys(&swkbd, L'.', 0);
							//Set the initial text, so I want to set that to what's already in the IP Addr string so we can ammend it
							//And not need to write from the start every time
							swkbdSetInitialText(&swkbd, targetIPAddress);
							//Away we go
							swkbdInputText(&swkbd, targetIPAddress, sizeof(targetIPAddress));
							strcatToConnConsole("Set IP :");
							strcatToConnConsole(targetIPAddress);
							strcatToConnConsole("\n");
						}
					}
					break;
				case BOT_TAB_STATE_CONFIG:
					//these set flags that are used to set animations for the buttons on this screen
					if ((touch.px > CFG_SCR_BUTT_CPAD_VERT_TOUCH_X) && (touch.px < CFG_SCR_BUTT_CPAD_VERT_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_CPAD_VERT_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_CPAD_VERT_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentAxisControl = &cpadVertAxis;			
						configCVIsAxis = true;			
					}	

					if ((touch.px > CFG_SCR_BUTT_CPAD_HORI_TOUCH_X) && (touch.px < CFG_SCR_BUTT_CPAD_HORI_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_CPAD_HORI_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_CPAD_HORI_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentAxisControl = &cpadHoriAxis;									
						configCVIsAxis = true;
					}	

					if ((touch.px > CFG_SCR_BUTT_DPAD_VERT_TOUCH_X) && (touch.px < CFG_SCR_BUTT_DPAD_VERT_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_DPAD_VERT_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_DPAD_VERT_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentAxisControl = &dpadVertAxis;					
						configCVIsAxis = true;				
					}

					if ((touch.px > CFG_SCR_BUTT_DPAD_HORI_TOUCH_X) && (touch.px < CFG_SCR_BUTT_DPAD_HORI_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_DPAD_HORI_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_DPAD_HORI_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentAxisControl = &dpadHoriAxis;					
						configCVIsAxis = true;			
					}

					if ((touch.px > CFG_SCR_BUTT_L_TOUCH_X) && (touch.px < CFG_SCR_BUTT_L_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_L_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_L_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentPBControl = &lPushbutton;					
						configCVIsAxis = false;										
					}

					if ((touch.px > CFG_SCR_BUTT_A_TOUCH_X) && (touch.px < CFG_SCR_BUTT_A_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_A_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_A_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentPBControl = &aPushbutton;					
						configCVIsAxis = false;		
									
					}

					if ((touch.px > CFG_SCR_BUTT_B_TOUCH_X) && (touch.px < CFG_SCR_BUTT_B_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_B_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_B_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentPBControl = &bPushbutton;					
						configCVIsAxis = false;				
										
					}

					if ((touch.px > CFG_SCR_BUTT_X_TOUCH_X) && (touch.px < CFG_SCR_BUTT_X_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_X_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_X_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentPBControl = &xPushbutton;					
						configCVIsAxis = false;					
										
					}

					if ((touch.px > CFG_SCR_BUTT_Y_TOUCH_X) && (touch.px < CFG_SCR_BUTT_Y_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_Y_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_Y_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentPBControl = &yPushbutton;					
						configCVIsAxis = false;					
					}

					if ((touch.px > CFG_SCR_BUTT_R_TOUCH_X) && (touch.px < CFG_SCR_BUTT_R_TOUCH_X + CFG_SCR_BUTT_CONTROL_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_R_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_R_TOUCH_Y + CFG_SCR_BUTT_CONTROL_SIZE))			//are within the hitbox that I defined for the button
					{
						currentPBControl = &rPushbutton;					
						configCVIsAxis = false;					
					}

					if ((touch.px > CFG_SCR_TICKBOX_TOUCH_X) && (touch.px < CFG_SCR_TICKBOX_TOUCH_X + CFG_SCR_TICKBOX_SCALE_SIZE) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_TICKBOX_TOUCH_Y) && (touch.py < CFG_SCR_TICKBOX_TOUCH_Y + CFG_SCR_TICKBOX_SCALE_SIZE))			//are within the hitbox that I defined for the button
					{
						if (configCVIsAxis)
						{
							currentAxisControl->sprungToCentre = (!currentAxisControl->sprungToCentre);	
						}	
						else
						{
							currentPBControl->toggle = (!currentPBControl->toggle);
						}								
					}

					if ((touch.px > CFG_SCR_BUTT_SETCV_TOUCH_X) && (touch.px < CFG_SCR_BUTT_SETCV_TOUCH_X + CFG_SCR_BUTT_SETCV_SIZE_X) &&	//Looks scary but basically I just check if the X and Y values
					(touch.py > CFG_SCR_BUTT_SETCV_TOUCH_Y) && (touch.py < CFG_SCR_BUTT_SETCV_TOUCH_Y + CFG_SCR_BUTT_SETCV_SIZE_Y))			//are within the hitbox that I defined for the button
					{
						if (configCVIsAxis)
						{
							//currentAxisControl->sprungToCentre = (!currentAxisControl->sprungToCentre);
							strcatToConnConsole("SetCV got pressed!\n")	;
							handleSettingCVForCurrentStruct();
						}	
						else
						{
							strcatToConnConsole("SetCV got pressed!\n")	;
							handleSettingCVForCurrentStruct();
						}								
					}
					break;
			}
		}

		//Handle the cv list scrolling
		if (CV_SCR_LIST_SPIRTE_Y > CV_SCR_LIST_SPIRTE_BASE_Y)
		{
			if (testCVListBasePointer > 6 )
			{
				CV_SCR_LIST_SPIRTE_Y -= CV_SCR_LIST_SPIRTE_SPACING;			
				testCVListBasePointer--;
			}
			else
			{
				canDnScroll = false;
			}
		}

		if (CV_SCR_LIST_SPIRTE_Y < (CV_SCR_LIST_SPIRTE_BASE_Y - CV_SCR_LIST_SPIRTE_SPACING) )
		{
			if (testCVListBasePointer < cvListSize )
			{
				CV_SCR_LIST_SPIRTE_Y += CV_SCR_LIST_SPIRTE_SPACING;			
				testCVListBasePointer++;
			}
			else
			{
				canUpScroll = false;
			}
		}

		//Clone current state into old state
		memcpy(&oldTouch, &touch, sizeof(touch) );

		//Handle animations
		//---------------------

		C2D_SpriteSetPos(&botScrUISprites[connScrButtConnect].spr, (touchButtons.connScrConnect) ? CON_SCR_BUT_CONNECT_X + TOUCH_SCR_CLICK_ANIM_X : CON_SCR_BUT_CONNECT_X, 
		(touchButtons.connScrConnect) ? CON_SCR_BUT_CONNECT_Y + TOUCH_SCR_CLICK_ANIM_Y : CON_SCR_BUT_CONNECT_Y);

		C2D_SpriteSetPos(&botScrUISprites[connScrButtDisconnect].spr, (touchButtons.connScrDisconnect) ? CON_SCR_BUT_DISCONNECT_X + TOUCH_SCR_CLICK_ANIM_X : CON_SCR_BUT_DISCONNECT_X, 
		(touchButtons.connScrDisconnect) ? CON_SCR_BUT_DISCONNECT_Y + TOUCH_SCR_CLICK_ANIM_Y : CON_SCR_BUT_DISCONNECT_Y);

		C2D_SpriteSetPos(&botScrUISprites[botTabButtConn].spr, BOT_TAB_CONN_X, 
		(botScreenState == BOT_TAB_STATE_CONNECTION) ? BOT_TAB_CONN_Y + BOT_TAB_ANIM_Y : BOT_TAB_CONN_Y);

		C2D_SpriteSetPos(&botScrUISprites[botTabButtCV].spr, BOT_TAB_CONTROLS_X, 
		(botScreenState == BOT_TAB_STATE_CONTROLVALUES) ? BOT_TAB_CONTROLS_Y + BOT_TAB_ANIM_Y : BOT_TAB_CONTROLS_Y);

		C2D_SpriteSetPos(&botScrUISprites[connScrButtIPAddr].spr, (touchButtons.connScrIPAddr) ? CON_SCR_BUT_IPADDR_X + TOUCH_SCR_CLICK_ANIM_X : CON_SCR_BUT_IPADDR_X, 
		(touchButtons.connScrIPAddr) ? CON_SCR_BUT_IPADDR_Y + TOUCH_SCR_CLICK_ANIM_Y : CON_SCR_BUT_IPADDR_Y);

		C2D_SpriteSetPos(&botScrUISprites[botTabButtCfg].spr, BOT_TAB_CONFIG_X, 
		(botScreenState == BOT_TAB_STATE_CONFIG) ? BOT_TAB_CONFIG_Y + BOT_TAB_ANIM_Y : BOT_TAB_CONFIG_Y);

		//Handle the buttons on the config screen
		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtCpadVert].spr, (touchButtons.cfgScrCPADVert) ? CFG_SCR_BUTT_CPAD_VERT_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_CPAD_VERT_X, 
		(touchButtons.cfgScrCPADVert) ? CFG_SCR_BUTT_CPAD_VERT_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_CPAD_VERT_Y);
		
		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtCpadHori].spr, (touchButtons.cfgScrCPADHori) ? CFG_SCR_BUTT_CPAD_HORI_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_CPAD_HORI_X, 
		(touchButtons.cfgScrCPADHori) ? CFG_SCR_BUTT_CPAD_HORI_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_CPAD_HORI_Y);
		

		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtDpadVert].spr, (touchButtons.cfgScrDPADVert) ? CFG_SCR_BUTT_DPAD_VERT_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_DPAD_VERT_X, 
		(touchButtons.cfgScrDPADVert) ? CFG_SCR_BUTT_DPAD_VERT_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_DPAD_VERT_Y);

		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtDpadHori].spr, (touchButtons.cfgScrDPADHori) ? CFG_SCR_BUTT_DPAD_HORI_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_DPAD_HORI_X, 
		(touchButtons.cfgScrDPADHori) ? CFG_SCR_BUTT_DPAD_HORI_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_DPAD_HORI_Y);

		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtL].spr, (touchButtons.cfgScrL) ? CFG_SCR_BUTT_L_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_L_X, 
		(touchButtons.cfgScrL) ? CFG_SCR_BUTT_L_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_L_Y);

		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtA].spr, (touchButtons.cfgScrA) ? CFG_SCR_BUTT_A_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_A_X, 
		(touchButtons.cfgScrA) ? CFG_SCR_BUTT_A_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_A_Y);

		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtB].spr, (touchButtons.cfgScrB) ? CFG_SCR_BUTT_B_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_B_X, 
		(touchButtons.cfgScrB) ? CFG_SCR_BUTT_B_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_B_Y);

		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtX].spr, (touchButtons.cfgScrX) ? CFG_SCR_BUTT_X_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_X_X, 
		(touchButtons.cfgScrX) ? CFG_SCR_BUTT_X_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_X_Y);

		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtY].spr, (touchButtons.cfgScrY) ? CFG_SCR_BUTT_Y_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_Y_X, 
		(touchButtons.cfgScrY) ? CFG_SCR_BUTT_Y_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_Y_Y);

		C2D_SpriteSetPos(&botScrUISprites[cfgScrButtR].spr, (touchButtons.cfgScrR) ? CFG_SCR_BUTT_R_X + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_R_X, 
		(touchButtons.cfgScrR) ? CFG_SCR_BUTT_R_Y + CFG_SCR_BUTT_ANIM : CFG_SCR_BUTT_R_Y);
		
		

		//Place all the sprites for the CV List by using an offset from the base position.
		C2D_SpriteSetPos(&botScrUISprites[cvListSprite1].spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y);
		C2D_SpriteSetPos(&botScrUISprites[cvListSprite2].spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 1));
		C2D_SpriteSetPos(&botScrUISprites[cvListSprite3].spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 2));
		C2D_SpriteSetPos(&botScrUISprites[cvListSprite4].spr, CV_SCR_LIST_SPIRTE_X , CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 3));
		C2D_SpriteSetPos(&botScrUISprites[cvListSprite5].spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 4));
		C2D_SpriteSetPos(&botScrUISprites[cvListSprite6].spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 5));
		C2D_SpriteSetPos(&botScrUISprites[cvListSprite7].spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 6));
		
		
		//Example info printfs
		//printf("\x1b[1;1HSprites: %zu/%u\x1b[K", numSprites, MAX_SPRITES);
		//printf("\x1b[2;1HCPU:     %6.2f%%\x1b[K", C3D_GetProcessingTime()*6.0f);
		//printf("\x1b[3;1HGPU:     %6.2f%%\x1b[K", C3D_GetDrawingTime()*6.0f);
		//printf("\x1b[4;1HCmdBuf:  %6.2f%%\x1b[K", C3D_GetCmdBufUsage()*100.0f);
		//printf("\x1b[5;1HSprite Pos:  X=%.2f Y=%.2f\x1b[K", spriteX, spriteY);

		//-------------------
		//	Screen rendering
		//-------------------

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		//-------------------
		//	Draw my top screen
		//-------------------

		C2D_TargetClear(top, C2D_Color32(BGCOLOUR_R, BGCOLOUR_G, BGCOLOUR_B, BGCOLOUR_A)); //Clear with blue BG				
		C2D_SceneBegin(top);

		//Draw Top screen UI bar
		C2D_DrawRectSolid( TOP_TOPBARCOORD_X, TOP_TOPBARCOORD_Y, TOP_TOPBARCOORD_Z, TOP_SCREEN_WIDTH, TOP_TOPBARHEIGHT
		, C2D_Color32( TOP_TOPBARCOLOUR_R, TOP_TOPBARCOLOUR_G, TOP_TOPBARCOLOUR_B, TOP_TOPBARCOLOUR_A)); //Top bar rectangle

		//Render UI sprites
		for (size_t i = 0; i < TOP_UISPRITECOUNT ; i ++)
		{
			//Various states to handle hiding UI elements.
			switch (i) //Switch on what sprite we're on, then determine if it needs to be rendered or not.
			{
				case tsControlState:
					if (!currentControlState)
						continue;
					break;

				case dpadUpSprite:
					if (!(kHeld & KEY_DUP) || !currentControlState)
						continue;
					break;
				case dpadDownSprite:
					if (!(kHeld & KEY_DDOWN) || !currentControlState)
						continue;
					break;
				case dpadLeftSprite:
					if (!(kHeld & KEY_DLEFT) || !currentControlState)
						continue;
					break;
				case dpadRightSprite:
					if (!(kHeld & KEY_DRIGHT) || !currentControlState)
						continue;
					break;
				case cpadUpSprite:
					if (!(kHeld & KEY_CPAD_UP) || !currentControlState)
						continue;
					break;
				case cpadDownSprite:
					if (!(kHeld & KEY_CPAD_DOWN) || !currentControlState)
						continue;
					break;
				case cpadLeftSprite:
					if (!(kHeld & KEY_CPAD_LEFT) || !currentControlState)
						continue;
					break;
				case cpadRightSprite:
					if (!(kHeld & KEY_CPAD_RIGHT) || !currentControlState)
						continue;
					break;
				case wifi_0:
					if (wifiState != WIFI_LEVEL_0)
						continue;
					break;
				case wifi_1:
					if (wifiState != WIFI_LEVEL_1)
						continue;
					break;
				case wifi_2:
					if (wifiState != WIFI_LEVEL_2)
						continue;
					break;
				case wifi_3:
					if (wifiState != WIFI_LEVEL_3)
						continue;
					break;
			}

			C2D_DrawSprite(&topScrUISprites[i].spr);
		}
		

		//Render Static text elements	
		C2D_DrawText( &gStaticText[0], C2D_WithColor | C2D_AlignCenter , TOP_SCREEN_WIDTH / 2 , STRING_TITLE_Y, 0, 0.5, 0.5, C2D_Color32(255,255,255,255) );

		//-------------------
		//	Draw my bottom screen
		//-------------------

		C2D_TargetClear(bot, C2D_Color32(BGCOLOUR_R, BGCOLOUR_G, BGCOLOUR_B, BGCOLOUR_A)); //Clear with blue BG
		C2D_SceneBegin(bot);

		//Draw Top screen UI bar
		C2D_DrawRectSolid( BOT_TOPBARCOORD_X, BOT_TOPBARCOORD_Y, BOT_TOPBARCOORD_Z, BOT_SCREEN_WIDTH, BOT_TOPBARHEIGHT
		, C2D_Color32( BOT_TOPBARCOLOUR_R, BOT_TOPBARCOLOUR_G, BOT_TOPBARCOLOUR_B, BOT_TOPBARCOLOUR_A));

		switch (botScreenState)
		{
			case BOT_TAB_STATE_CONNECTION:

				drawConnConsole(false);
				break;

			case BOT_TAB_STATE_CONTROLVALUES:

				break;

		}	

		//Render UI sprites
		for (size_t i = 0; i < BOT_UISPRITECOUNT ; i ++)
		{
			//Various states to handle hiding UI elements.
					
			switch (i) //Switch on what sprite we're on, then determine if it needs to be rendered or not.
			{
				case connScrButtConnect:	//INTENTIONAL FALLTHROUGH
				case connScrButtIPAddr:
				case connScrButtDisconnect:					
					if (botScreenState != BOT_TAB_STATE_CONNECTION)
						continue;
					break; 

				case cvListSprite1:		//INTENTIONAL FALLTHROUGH
					if (testCVListBasePointer == cvListSize)
						continue;
				case cvListSprite2:
				case cvListSprite3:
				case cvListSprite4:
				case cvListSprite5:
				case cvListSprite6:
				case cvListSprite7:
					if (botScreenState != BOT_TAB_STATE_CONTROLVALUES)
						continue;
					break;

				//Control the tick box rendering
				case cfgScrTickboxTicked: //INTENTIONAL FALLTHROUGH
					if(configCVIsAxis) //Make sure we look into the right type of struct
					{
						if (!currentAxisControl->sprungToCentre) //If it's not spring to centre, yeet
							continue;
					}
					else
					{
						if (!currentPBControl->toggle) //If it's not spring to centre, yeet
							continue;
					}
				case cfgScrTickbox:				//INTENTIONAL FALLTHROUGH
				case cfgScrCFGArea:
				case cfgScrButtCpadVert:
				case cfgScrButtCpadHori:
				case cfgScrButtDpadVert:
				case cfgScrButtDpadHori:
				case cfgScrButtL:
				case cfgScrButtA:
				case cfgScrButtB:
				case cfgScrButtX:
				case cfgScrButtY:
				case cfgScrButtR:
				case cfgScrButtSetCV:				
					if (botScreenState != BOT_TAB_STATE_CONFIG)
						continue;
					break;
			}	

			switch (i) //This handles greying out of buttons
			{
				case connScrButtConnect:	//INTENTIONAL FALLTHROUGH
				case connScrButtIPAddr:
					if (networkingConnected)
					{
						C2D_ImageTint tint;
						C2D_AlphaImageTint(&tint, GREY_OUT_TINT);
						C2D_DrawSpriteTinted(&botScrUISprites[i].spr,&tint);
						continue;
					}
					break;
				case connScrButtDisconnect:					
					if (!networkingConnected)
						{
							C2D_ImageTint tint;
							C2D_AlphaImageTint(&tint, GREY_OUT_TINT);
							C2D_DrawSpriteTinted(&botScrUISprites[i].spr,&tint);
							continue;
						}
					break;

				default:
					break;
			}


			C2D_DrawSprite(&botScrUISprites[i].spr);
		}

		//Deferred drawing

		if (botScreenState == BOT_TAB_STATE_CONTROLVALUES)
		{			
			//Drawing the text for the CV displays
			/*
			{
				C2D_Text consoleText;
				dynamicTextCreate(&consoleText, testCVList[testCVListBasePointer - 6]);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *6), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				dynamicTextCreate(&consoleText, testCVList[testCVListBasePointer - 5]);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *5), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				dynamicTextCreate(&consoleText, testCVList[testCVListBasePointer - 4]);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *4), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				dynamicTextCreate(&consoleText, testCVList[testCVListBasePointer - 3]);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *3), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				dynamicTextCreate(&consoleText, testCVList[testCVListBasePointer - 2]);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *2), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				dynamicTextCreate(&consoleText, testCVList[testCVListBasePointer - 1]);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *1), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{ //Text for base sprite
				C2D_Text consoleText;
				dynamicTextCreate(&consoleText, testCVList[testCVListBasePointer]);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}	
			*/	

			char parseBuf[CV_LIST_CV_STRLEN] = {'\0'};
			{
				C2D_Text consoleText;
				sprintf(parseBuf , "%s\nMin = %.2f Max = %.2f", cvList[testCVListBasePointer - 6].name, cvList[testCVListBasePointer - 6].min, cvList[testCVListBasePointer - 6].max);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *6), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				sprintf(parseBuf , "%s\nMin = %.2f Max = %.2f", cvList[testCVListBasePointer - 5].name, cvList[testCVListBasePointer - 5].min, cvList[testCVListBasePointer - 5].max);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *5), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				sprintf(parseBuf , "%s\nMin = %.2f Max = %.2f", cvList[testCVListBasePointer - 4].name, cvList[testCVListBasePointer - 4].min, cvList[testCVListBasePointer - 4].max);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *4), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				sprintf(parseBuf , "%s\nMin = %.2f Max = %.2f", cvList[testCVListBasePointer - 3].name, cvList[testCVListBasePointer - 3].min, cvList[testCVListBasePointer - 3].max);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *3), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				sprintf(parseBuf , "%s\nMin = %.2f Max = %.2f", cvList[testCVListBasePointer - 2].name, cvList[testCVListBasePointer - 2].min, cvList[testCVListBasePointer - 2].max);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *2), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{
				C2D_Text consoleText;
				sprintf(parseBuf , "%s\nMin = %.2f Max = %.2f", cvList[testCVListBasePointer - 1].name, cvList[testCVListBasePointer - 1].min, cvList[testCVListBasePointer - 1].max);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y - (CV_SCR_LIST_SPIRTE_SPACING *1), 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
			{ //Text for base sprite
				C2D_Text consoleText;
				sprintf(parseBuf , "%s\nMin = %.2f Max = %.2f", cvList[testCVListBasePointer - 0].name, cvList[testCVListBasePointer - 0].min, cvList[testCVListBasePointer - 0].max);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LIST_TEXT_X , CV_SCR_LIST_TEXT_Y, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}	

			//Cover bar covers the top of the list.
			C2D_DrawRectSolid( CV_SCR_COVER_BARCOORD_X, CV_SCR_COVER_BARCOORD_Y, CV_SCR_COVER_BARCOORD_Z, BOT_SCREEN_WIDTH, CV_SCR_COVER_BARHEIGHT
			, C2D_Color32( CV_SCR_COVER_BARCOLOUR_R, CV_SCR_COVER_BARCOLOUR_G, CV_SCR_COVER_BARCOLOUR_B, CV_SCR_COVER_BARCOLOUR_A));

			{ //Text for base loconame
				C2D_Text consoleText;
				dynamicTextCreate(&consoleText, locoNamePointer);
				C2D_DrawText( &consoleText, C2D_WithColor, CV_SCR_LOCONAME_TXT_X , CV_SCR_LOCONAME_TXT_Y, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
			}
		}

		if (botScreenState == BOT_TAB_STATE_CONFIG)
		{

			char parseBuf[255] = {'\0'};
			if (configCVIsAxis)
			{ 
				C2D_Text consoleText;
				sprintf(parseBuf, "Configuring : %s", currentAxisControl->axisName);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor | C2D_AlignCenter , CFG_SCR_AXIS_NAME_TEXT_X , CFG_SCR_AXIS_NAME_TEXT_Y, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
				parseBuf[0] = '\0';

				if (!currentAxisControl->hasValidCV)
					sprintf(parseBuf, "Has valid control value : No\n\nSprung to centre : \n\nSet control value name :");
				else
					sprintf(parseBuf, "Has valid control value : Yes\n\nSprung to centre : \n\nSet control value name :\n\n\n\n Current control value is : \n    %s", currentAxisControl->control->name);

				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor , CFG_SCR_AXIS_CONFIG_TEXT_X , CFG_SCR_AXIS_CONFIG_TEXT_Y, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
				//C2D_DrawText( &consoleText, C2D_WithColor , 0 , 0, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
				parseBuf[0] = '\0';

			}
			else
			{
				C2D_Text consoleText;
				sprintf(parseBuf, "Configuring : %s", currentPBControl->pushbuttonName);
				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor | C2D_AlignCenter , CFG_SCR_AXIS_NAME_TEXT_X , CFG_SCR_AXIS_NAME_TEXT_Y, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
				parseBuf[0] = '\0';

				if (!currentPBControl->hasValidCV)
					sprintf(parseBuf, "Has valid control value : No\n\nButton is toggle  : \n\nSet control value name :");
				else
					sprintf(parseBuf, "Has valid control value : Yes\n\nButton is toggle  : \n\nSet control value name :\n\n\n\n Current control value is : \n    %s", currentPBControl->control->name);

				dynamicTextCreate(&consoleText, parseBuf);
				C2D_DrawText( &consoleText, C2D_WithColor , CFG_SCR_AXIS_CONFIG_TEXT_X , CFG_SCR_AXIS_CONFIG_TEXT_Y, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
				//C2D_DrawText( &consoleText, C2D_WithColor , 0 , 0, 0, 0.5, 0.5, COLOUR_C32_BLACK );	
				parseBuf[0] = '\0';	
			}
		}
		
		C3D_FrameEnd(0);
	}

	// Delete graphics
	C2D_SpriteSheetFree(topScrSpriteSheet);
	C2D_SpriteSheetFree(botScrSpriteSheet);

	// Deinit libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}

//Function to create and initialise all of our UI sprites.
static void createTopScreenUISprites() 
{
	//Init all the uisprite structs as undefined to start
	for (size_t i = 0; i < TOP_UISPRITECOUNT; i++)
		topScrUISprites[i].def = false;

	//size_t numImages = C2D_SpriteSheetCount(topScrSpriteSheet);

	for (size_t i = 0; i < TOP_UISPRITECOUNT; i++)
	{
		UISprite* sprite = &topScrUISprites[i];

		// Grab images from the sheet, uiSpriteList[] lists sprites and allows for reuse, IE the CPad U,D,L & R are all reused from the Dpad
		// so the array allows us the load them from their position in the sheet without having to add them twice to the list.
		C2D_SpriteFromSheet(&sprite->spr, topScrSpriteSheet, topUISpriteList[i]);
		C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);

		switch (i) //Uses intentional fallthrough
		{
			case tsControlState:
				C2D_SpriteSetPos(&sprite->spr, (int)TOP_SCREEN_WIDTH * UI_CONSTATE_X, (int)TOP_SCREEN_HEIGHT * UI_CONSTATE_Y);
				printf("\x1b[%d;1HControlStateSprite", 11+i);
				break;

			case dpadSprite: /* FALLTHROUGH */
			case dpadUpSprite:
			case dpadDownSprite:
			case dpadLeftSprite:
			case dpadRightSprite:
				C2D_SpriteSetPos(&sprite->spr, (int)TOP_SCREEN_WIDTH * UI_DPAD_X, (int)TOP_SCREEN_HEIGHT * UI_DPAD_Y);
				printf("\x1b[%d;1HDpadSprite", 11+i);
				break;
			
			case cpadSprite: /* FALLTHROUGH */
			case cpadUpSprite:
			case cpadDownSprite:
			case cpadLeftSprite:
			case cpadRightSprite:
				C2D_SpriteSetPos(&sprite->spr, (int)TOP_SCREEN_WIDTH * UI_CPAD_X, (int)TOP_SCREEN_HEIGHT * UI_CPAD_Y);
				printf("\x1b[%d;1HCpadSprite", 11+i);
				break;

			case wifi_0:
			case wifi_1:
			case wifi_2:
			case wifi_3:
				C2D_SpriteSetPos(&sprite->spr, UI_WIFI_X, UI_WIFI_Y);
				//printf("\x1b[%d;1HCpadSprite", 11+i);
				break;


			default:
				C2D_SpriteSetPos(&sprite->spr, TOP_SCREEN_WIDTH * spriteX, TOP_SCREEN_HEIGHT  * spriteY);
				printf("\x1b[%d;1HOtherSprite", 11+i);
				break;
		}

		C2D_SpriteSetRotation(&sprite->spr, C3D_Angle(0));
		sprite->def = true;
	}
}

//Function to create and initialise all of our UI sprites.
static void createBotScreenUISprites() 
{
	//Init all the uisprite structs as undefined to start
	for (size_t i = 0; i < BOT_UISPRITECOUNT; i++)
		botScrUISprites[i].def = false;

	//size_t numImages = C2D_SpriteSheetCount(botScrSpriteSheet);

	for (size_t i = 0; i < BOT_UISPRITECOUNT; i++)
	{
		UISprite* sprite = &botScrUISprites[i];

		// Grab images from the sheet, uiSpriteList[] lists sprites and allows for reuse, IE the CPad U,D,L & R are all reused from the Dpad
		// so the array allows us the load them from their position in the sheet without having to add them twice to the list.
		C2D_SpriteFromSheet(&sprite->spr, botScrSpriteSheet, botUISpriteList[i]);
		C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);

		switch (i) //Uses intentional fallthrough
		{
			case connScrButtConnect:
				C2D_SpriteSetPos(&sprite->spr, CON_SCR_BUT_CONNECT_X, CON_SCR_BUT_CONNECT_Y);
				//printf("\x1b[%d;1HControlStateSprite", 11+i);
				break;

			case connScrButtDisconnect:
				C2D_SpriteSetPos(&sprite->spr, CON_SCR_BUT_DISCONNECT_X, CON_SCR_BUT_DISCONNECT_Y);
				//printf("\x1b[%d;1HControlStateSprite", 11+i);
				break;

			case botTabButtConn:
				C2D_SpriteSetPos(&sprite->spr, BOT_TAB_CONN_X, BOT_TAB_CONN_Y);
				break;

			case botTabButtCV:
				C2D_SpriteSetPos(&sprite->spr, BOT_TAB_CONTROLS_X, BOT_TAB_CONTROLS_Y);
				break;

			case connScrButtIPAddr:
				C2D_SpriteSetPos(&sprite->spr, CON_SCR_BUT_IPADDR_X, CON_SCR_BUT_IPADDR_Y);
				break;

			case cvListSprite1:
				C2D_SpriteSetPos(&sprite->spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y);		
				break;
			case cvListSprite2:
				C2D_SpriteSetPos(&sprite->spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 1));	
				break;
			case cvListSprite3:
				C2D_SpriteSetPos(&sprite->spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 2));		
				break;
			case cvListSprite4:
				C2D_SpriteSetPos(&sprite->spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 3));	
				break;
			case cvListSprite5:
				C2D_SpriteSetPos(&sprite->spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 4));	
				break;
			case cvListSprite6:
				C2D_SpriteSetPos(&sprite->spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 5));	
				break;
			case cvListSprite7:
				C2D_SpriteSetPos(&sprite->spr, CV_SCR_LIST_SPIRTE_X, CV_SCR_LIST_SPIRTE_Y - (CV_SCR_LIST_SPIRTE_SPACING * 6));	
				break;

			case botTabButtCfg:
				C2D_SpriteSetPos(&sprite->spr, BOT_TAB_CONFIG_X, BOT_TAB_CONFIG_Y );	
				break;

			case cfgScrCFGArea:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_CFGAREA_SPIRTE_X, CFG_SCR_CFGAREA_SPIRTE_Y );	
				break;

			case cfgScrButtCpadVert:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_CPAD_VERT_X, CFG_SCR_BUTT_CPAD_VERT_Y );	
				break;

			case cfgScrButtCpadHori:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_CPAD_HORI_X, CFG_SCR_BUTT_CPAD_HORI_Y );	
				break;

			case cfgScrButtDpadVert:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_DPAD_VERT_X, CFG_SCR_BUTT_DPAD_VERT_Y );	
				break;

			case cfgScrButtDpadHori:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_DPAD_HORI_X, CFG_SCR_BUTT_DPAD_HORI_Y );	
				break;

			case cfgScrButtL:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_L_X, CFG_SCR_BUTT_L_Y );	
				break;

			case cfgScrButtA:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_A_X, CFG_SCR_BUTT_A_Y );	
				break;

			case cfgScrButtB:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_B_X, CFG_SCR_BUTT_B_Y );	
				break;

			case cfgScrButtX:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_X_X, CFG_SCR_BUTT_X_Y );	
				break;

			case cfgScrButtY:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_Y_X, CFG_SCR_BUTT_Y_Y );	
				break;

			case cfgScrButtR:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_R_X, CFG_SCR_BUTT_R_Y );	
				break;

			case cfgScrTickbox:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_TICKBOX_X, CFG_SCR_TICKBOX_Y );	
				C2D_SpriteScale (&sprite->spr, 2, 2);
				break;

			case cfgScrTickboxTicked:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_TICKBOX_X, CFG_SCR_TICKBOX_Y );	
				C2D_SpriteScale (&sprite->spr, 2, 2);
				break;

			case cfgScrButtSetCV:
				C2D_SpriteSetPos(&sprite->spr, CFG_SCR_BUTT_SETCV_X, CFG_SCR_BUTT_SETCV_Y );	
				break;

			default:
				C2D_SpriteSetPos(&sprite->spr, BOT_SCREEN_WIDTH * spriteX, BOT_SCREEN_HEIGHT  * spriteY);
				printf("\x1b[%d;1HOtherSprite", 11+i);
				break;
		}

		C2D_SpriteSetRotation(&sprite->spr, C3D_Angle(0));
		sprite->def = true;
	}
}

//function to init our static text elements
static void initTextRender()
{
	gStaticTextBuffer = C2D_TextBufNew(4096);
	gDynamicTextBuffer = C2D_TextBufNew(4096);

	C2D_TextParse(&gStaticText[0], gStaticTextBuffer, STRING_TITLE_S);

	C2D_TextOptimize(&gStaticText[0]);
}

//Function to write to connection screen console, Handles scrolling.
static void strcatToConnConsole(char* source)
{
	strcat(connConsoleString, source);

	int newLineCharCnt = 0, firstNLPos = -1;
	for(int i = 0; i < strlen(connConsoleString); i++)
		if (connConsoleString[i] == '\n')
		{
			if (firstNLPos == -1)
				firstNLPos = i; 
			newLineCharCnt++;
		}

	if (newLineCharCnt > CON_SCR_MAX_NL_CHAR)
	{
		/*
		int firstNLPos;
		for(firstNLPos = 0; newLineCharCnt < strlen(connConsoleString); firstNLPos++)
			if (connConsoleString[firstNLPos] == '\n')
				break;
		*/
		firstNLPos++;
		memmove(connConsoleString, connConsoleString + firstNLPos, strlen(connConsoleString + firstNLPos) + 1);
	}
	return;
}

//Handles text buffer, parsing and optimisation in a single call
static void dynamicTextCreate(C2D_Text *text, char* string)
{
	C2D_TextBufClear(gDynamicTextBuffer);
	C2D_TextParse(text, gDynamicTextBuffer, string);
	C2D_TextOptimize(text);
	return;
}

static void drawConnConsole( bool redrawing) //Allows a redraw of just the connection console for use when connecting so the screen stays updated
{
	if (redrawing) // Removes the need to duplicate this in the main render cycle
	{
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_SceneBegin(bot);
	}

	// Draw the big grey square
	C2D_DrawRectSolid( CON_SCR_CONSCOORD_X, CON_SCR_CONSCOORD_Y, CON_SCR_CONSCOORD_Z, CON_SCR_CONSWIDTH, CON_SCR_CONSHEIGHT
	, C2D_Color32( CON_SCR_CONSCOLOUR_R, CON_SCR_CONSCOLOUR_G, CON_SCR_CONSCOLOUR_B, CON_SCR_CONSCOLOUR_A));

	//Text shit
	C2D_Text consoleText;
	dynamicTextCreate(&consoleText, connConsoleString);
	C2D_DrawText( &consoleText, C2D_WithColor, CON_SCR_TEXT_X , CON_SCR_TEXT_Y, 0, 0.5, 0.5, C2D_Color32(255,255,255,255) );
	//Au revoir
	if (redrawing)
		C3D_FrameEnd(0);		
	return;
}

static void populateCVList(char* list)
{
	char *str = strdup(list);  
	cvListSize = 0;  

	char *end_str;
    char *token = strtok_r(str, CV_LIST_BLOCK_DELIM, &end_str);

    for (int i = 0; token != NULL; i++)
    {
        char *end_token;
		/*
		{
			char temp[50];
			sprintf(temp, "a = %s\n", token);
        	strcatToConnConsole(temp);
		}
		*/
		cvList[i].isInited = true;
        char *token2 = strtok_r(token, CV_LIST_ELEM_DELIM, &end_token);
        for (int x = 0; token2 != NULL ; x++)
        {
			switch(x)
			{
				case 0:
					//strcpy(testCVList[i], token2);
					strcpy(cvList[i].name, token2);
					break;
				case 1:
					cvList[i].current = atof(token2);
					break;
				case 2:
					//strcat(testCVList[i], "\nMin : ");
					//strcat(testCVList[i], token2);
					cvList[i].min = atof(token2);
					break;
				case 3:
					//strcat(testCVList[i], " Max : ");
					//strcat(testCVList[i], token2);
					cvList[i].max = atof(token2);
					break;
			}
            //printf("b = %s\n", token2);
            token2 = strtok_r(NULL, CV_LIST_ELEM_DELIM, &end_token);
        }
        token = strtok_r(NULL, CV_LIST_BLOCK_DELIM, &end_str);
		cvListSize++;
    }
	{
		char temp[50];
		sprintf(temp, "%s%d\n", "CVList was : ",cvListSize );
		strcatToConnConsole(temp);
	}
	free(str);
    return ;
}

static void sendMessageViaSocket(char* inString)
{
	//Allocate some temp memory where we can add our end tag onto the string just incase the instring doesn't have enough memory assigned
	char* sendString = calloc(strlen(inString) + strlen(COMM_SYNTAX_TERM_MSG) + 10, sizeof(char));
	strcpy(sendString, inString);
	strcat(sendString, COMM_SYNTAX_TERM_MSG);

	int res = send (sock, sendString, strlen(sendString), 0);
	if (res < 0) {
		strcatToConnConsole("Server connection lost\n");
		networkingConnected = false;
		drawConnConsole(true);
		close(sock);
		strcatToConnConsole("Closed socket\n");
	}

	strcatToConnConsole("Msg sent over socket\n");
	free(sendString);
	return;
}

//function to loop recv() until message end tag located and store the full message in variable buf
static void recvCompleteMessageFromSockTobuf()
{
	//assign some temporary memory;
	char* readbuf = (char*) calloc(16386,sizeof(char));
	int bytesRecieved, numOfIterations = 0;
	//Loop to get full packets because a single call wouldn't recieve all of my messagexc nc|Z
	while((bytesRecieved = recv(sock, readbuf, BUFSIZE, 0)) > 0 )
	{
		numOfIterations++;
		strcatToConnConsole("CV list fragment recv()\n");
		//Add to buf, clear out the readbuf
		strcat(buf, readbuf);
		memset(readbuf, '\0', sizeof(readbuf));
		//Look for our end tag andbreak if found.
		if (strstr(buf, COMM_SYNTAX_TERM_MSG) != NULL)
			break;

		//Stops the application locking if no end tag is found.
		if (numOfIterations > 10)
		{
			strcatToConnConsole("WARN: Didn't Recv() end tag\n");
			return;
		}
	}
	//Chop off the end tag and free the memory, This would cause errors if no end tag was here hence the early return
	char* tempPointer = strstr(buf, COMM_SYNTAX_TERM_MSG);
	tempPointer[0] = '\0';
	free(readbuf);
	return;
}

void fetchAndHandleControlList()
{

	sendMessageViaSocket(COMM_SYNTAX_REQ_CVLIST);

	recvCompleteMessageFromSockTobuf();

	populateCVList(buf+strlen(COMM_SYNTAX_CVLIST));
	CV_SCR_LIST_SPIRTE_Y = ((BOT_SCREEN_HEIGHT + (CV_SCR_LIST_SPIRTE_SIZE_Y/2))+ CV_SCR_LIST_SPIRTE_SIZE_Y);
	testCVListBasePointer = 6;

	//Clear the buffer after use
	memset(buf, '\0', sizeof(buf));	

	return;
}

void fetchAndHandleLocoName(char* locoNamePointer)
{
	//Send message and wait for reply
	sendMessageViaSocket(COMM_SYNTAX_REQ_LOCONAME);
	recvCompleteMessageFromSockTobuf();

	//char debugbuff[128] = {'\0'};
	//strcpy(debugbuff , "Test.:.Debug.:.Name");
	//Create some pointers			
	char *token, *prov, *name, *temp;

	/// get the first token
	token = strtok(buf , ".:.");
	
	// walk through other tokens 
	for(int i = 0; token != NULL ; i++) 
	{
		switch(i) //Separate out the parts based on the iteration
		{
			case 0:
				prov = strdup(token);
				break;
			case 2:
				name = strdup(token);
				break;
		}					
		token = strtok(NULL, ".:.");
	}

	//Create a temporary buffer for the new loco name stuff with some spare
	temp = (char*) calloc(strlen(prov)+strlen(name)+10, sizeof(char));
	sprintf(temp, "Loco : %s\n%s", prov, name);

	//Realloc memory at the pointer to store our new string and store it there
	//locoNamePointer = (char*) realloc(locoNamePointer, sizeof(temp)+sizeof(char));
	strcpy(locoNamePointer, temp);
	
	//Log
	strcatToConnConsole("Fetched Loco Name\n");	

	//Cleanup
	free(temp);
	free(prov);
	free(name);

	//Clear the buffer after use
	//memset(buf, '\0', sizeof(buf));
	buf[0] = '\0';
	return;
}

//Returns Control Value struct to a base known state
void resetControlValueStruct(controlValue* cv)
{
	cv->isInited = false;
	cv->name[0] = '\0';
	cv->current= cv->currentOld = cv->min = cv->max = 0;
	return;
}

void setControlValueStruct(controlValue* cv, char* setCVName, float setCurr, float setMin, float setMax)
{
	cv->isInited = true;
	strcpy(cv->name, setCVName);
	cv->current = setCurr;
	cv->currentOld = cv->current;
	cv->min = setMin;
	cv->max = setMax;
	return;
}

void springControlBackToCentre(controlValue* cv)
{
	float midPoint = cv->min + ((cv->max - cv->min)/2);

	if (cv->current == midPoint)
		return;

	if (cv->current > midPoint)
	{
		cv->current = ((cv->current - SPRING_TO_CENTRE_SENSIT) > midPoint) ? midPoint : (cv->current - SPRING_TO_CENTRE_SENSIT);
	}
	else
	{
		cv->current = ((cv->current + SPRING_TO_CENTRE_SENSIT) < midPoint) ? midPoint : (cv->current + SPRING_TO_CENTRE_SENSIT);
	}

}

float returnCVCentre(controlValue* cv)
{
	float working = ((cv->max + cv->min)/2);
	char stagingBuf[57] = {'\0'};
	sprintf(stagingBuf, "Mid was %f\n", working);
	strcatToConnConsole(stagingBuf);
	return working;
}

void resetAxisControlStruct(axisControl* axis)
{
	axis->hasValidCV = false;
	axis->control = NULL;
	axis->sprungToCentre = false;
	return;
}

void setAxisControlStruct(axisControl* axis, controlValue* cv)
{
	axis->hasValidCV = true;
	axis->control = cv;
	//axis->sprungToCentre = false;
	return;
}

void resetPBControlStruct(pushbuttonControl* pb)
{
	pb->hasValidCV = false;
	pb->control = NULL;
	pb->toggle = false;
	return;
}

void setPBControlStruct(pushbuttonControl* pb, controlValue* cv)
{
	pb->hasValidCV = true;
	pb->control = cv;
	//pb->toggle = false;
	return;
}

void handleSettingCVForCurrentStruct()
{
	char swkbdTextBuffer[CV_LIST_CV_STRLEN] = {'\0'};
	//Create a software keyboard struct and initialise it
	SwkbdState swkbd;
	swkbdInit(&swkbd, SWKBD_TYPE_QWERTY, 1, CV_LIST_CV_STRLEN-1);
	//Various set features
	swkbdSetValidation(&swkbd, SWKBD_ANYTHING, 0, 0);
	swkbdSetFeatures(&swkbd,  SWKBD_DEFAULT_QWERTY );
	//Away we go
	swkbdInputText(&swkbd, swkbdTextBuffer, sizeof(swkbdTextBuffer));
	int i = 0, success = 0;
	for(; cvList[i].name[0] != '\0'; i++)
	{
		if (strcmp(cvList[i].name, swkbdTextBuffer) == 0)
		{
			success = 1;
			break;
		}
	}
	//We broke out the loop due to the for() condition and didn't find a match
	if (success == 0)
	{
		strcatToConnConsole("Did not find CV\n");
		return;
	}

	if (configCVIsAxis)
	{
		setAxisControlStruct(currentAxisControl, &cvList[i]);
	}
	else
	{
		setPBControlStruct(currentPBControl, &cvList[i]);
	}

	strcatToConnConsole("found CV\n");
	return;
}