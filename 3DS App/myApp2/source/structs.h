//----------------
//	STRUCTS / ENUMS
//----------------


//Original example Sprite struct (Should be deprecated)
typedef struct
{
	C2D_Sprite spr;
	float dx, dy; // velocity
} Sprite;

//Struct for UI Sprites
typedef struct
{
	C2D_Sprite spr; //sprite
	bool def;		//has this struct been defined (used so undefined structs are not used)
} UISprite;

//Used to store DPad and Circle Pad state in bitfields
struct touchButtonsPressed 
{
	bool connScrConnect : 1;
	bool connScrDisconnect : 1;
	bool connScrIPAddr : 1;
	bool cfgScrCPADVert : 1;
	bool cfgScrCPADHori : 1;
	bool cfgScrDPADVert : 1;
	bool cfgScrDPADHori : 1;
	bool cfgScrL : 1;
	bool cfgScrR : 1;
	bool cfgScrA : 1;
	bool cfgScrB : 1;
	bool cfgScrX : 1;
	bool cfgScrY : 1;
	bool dRi : 1;
	bool cUp : 1;
	bool cDn : 1;
	bool cLe : 1;
	bool cRi : 1;
};

//struct that stores control values
typedef struct
{
	bool isInited;
	char name[CV_LIST_CV_STRLEN];
	float current;
	float currentOld;
	float min;
	float max;
} controlValue;

#define AXISNAMELENGTHMAX 25
typedef struct 
{
	bool hasValidCV;
	char axisName[AXISNAMELENGTHMAX];
	controlValue* control;
	bool sprungToCentre;
} axisControl;

#define PBNAMELENGTHMAX 25
typedef struct 
{
	bool hasValidCV;
	char pushbuttonName[PBNAMELENGTHMAX];
	controlValue* control;
	bool toggle;
} pushbuttonControl;





//TOP SCREEN SPRITE INFO
//Enum stores the sprite position in the sprite list
enum topUISpriteListNums 
{
	sl_tsControlState = 0,
	sl_dpadSprite,
	sl_dpadUpSprite,
	sl_dpadDownSprite,
	sl_dpadLeftSprite,
	sl_dpadRightSprite,
	sl_cpadSprite,
    sl_wifi_0,
    sl_wifi_1,
    sl_wifi_2,
    sl_wifi_3
};  

//Enum stores the sprite number in the array of loaded sprites.
enum topUISpriteNums 
{
	tsControlState = 0,
	dpadSprite,
	dpadUpSprite,
	dpadDownSprite,
	dpadLeftSprite,
	dpadRightSprite,
	cpadSprite,
	cpadUpSprite,
	cpadDownSprite,
	cpadLeftSprite,
	cpadRightSprite,
    wifi_0,
    wifi_1,
    wifi_2,
    wifi_3
};

//This list is used to "instance" sprites, allowed reused sprites to be loaded multiple times for reuse.
int topUISpriteList[TOP_UISPRITECOUNT] = 
{
	sl_tsControlState,
	sl_dpadSprite,
	sl_dpadUpSprite,
	sl_dpadDownSprite,
	sl_dpadLeftSprite,
	sl_dpadRightSprite,
	sl_cpadSprite,
	sl_dpadUpSprite,
	sl_dpadDownSprite,
	sl_dpadLeftSprite,
	sl_dpadRightSprite,
    sl_wifi_0,
    sl_wifi_1,
    sl_wifi_2,
    sl_wifi_3
};

//BOTTOM SCREEN SPRITE INFO
enum botUISpritesListNums
{
	sl_bot_buttConnect = 0,
	sl_bot_buttDisconnect,
    sl_bot_tab_Connection,
	sl_bot_tab_ControlValues,
	sl_bot_buttIPAddr,
	sl_bot_CV_List,
	sl_bot_tab_config,
	sl_bot_cfg_cfgarea,
	sl_bot_cfg_buttCpadVert,
	sl_bot_cfg_buttCpadHori,
	sl_bot_cfg_buttDpadVert,
	sl_bot_cfg_buttDpadHori,
	sl_bot_cfg_buttL,
	sl_bot_cfg_buttA,
	sl_bot_cfg_buttB,
	sl_bot_cfg_buttX,
	sl_bot_cfg_buttY,
	sl_bot_cfg_buttR,
	sl_bot_tickbox,
	sl_bot_tickbox_ticked,
	sl_bot_buttSetCV,
};

enum botUISpriteNums
{
	connScrButtConnect = 0,
	connScrButtDisconnect,
    botTabButtConn,
	botTabButtCV,
	connScrButtIPAddr,
	cvListSprite1,
	cvListSprite2,
	cvListSprite3,
	cvListSprite4,
	cvListSprite5,
	cvListSprite6,
	cvListSprite7,
	botTabButtCfg,
	cfgScrCFGArea,
	cfgScrButtCpadVert,
	cfgScrButtCpadHori,
	cfgScrButtDpadVert,
	cfgScrButtDpadHori,
	cfgScrButtL,
	cfgScrButtA,
	cfgScrButtB,
	cfgScrButtX,
	cfgScrButtY,
	cfgScrButtR,
	cfgScrTickbox,
	cfgScrTickboxTicked,
	cfgScrButtSetCV,
};

int botUISpriteList[BOT_UISPRITECOUNT] = 
{
	sl_bot_buttConnect,
	sl_bot_buttDisconnect,
    sl_bot_tab_Connection,
	sl_bot_tab_ControlValues,
	sl_bot_buttIPAddr,
	sl_bot_CV_List,
	sl_bot_CV_List,
	sl_bot_CV_List,
	sl_bot_CV_List,
	sl_bot_CV_List,
	sl_bot_CV_List,
	sl_bot_CV_List,
	sl_bot_tab_config,
	sl_bot_cfg_cfgarea,
	sl_bot_cfg_buttCpadVert,
	sl_bot_cfg_buttCpadHori,
	sl_bot_cfg_buttDpadVert,
	sl_bot_cfg_buttDpadHori,
	sl_bot_cfg_buttL,
	sl_bot_cfg_buttA,
	sl_bot_cfg_buttB,
	sl_bot_cfg_buttX,
	sl_bot_cfg_buttY,
	sl_bot_cfg_buttR,
	sl_bot_tickbox,
	sl_bot_tickbox_ticked,
	sl_bot_buttSetCV,
};