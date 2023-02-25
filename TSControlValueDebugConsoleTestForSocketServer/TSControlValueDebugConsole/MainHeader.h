#pragma once
#pragma warning(disable: 4996)

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WS2tcpip.h>


#pragma comment (lib, "ws2_32.lib")

typedef void(*SetRailDriverConnectedType)(bool);
typedef char* (*GetControllerListType)(void);
typedef void (*SetControllerValueType)(int, float);
typedef float(*GetCurrentControllerValueType)(int);
typedef char* (*GetLocoNameType)(void);
typedef bool (*IsOverlayEnabledType)(void);
typedef float (*GetControllerValueType)(int, int);

#define COMM_SYNTAX_CVLIST          "CV_LISTING"
#define COMM_SYNTAX_LOCONAME        "LOCONAME"
#define COMM_SYNTAX_REQ_CVLIST      "REQ_CV_LIST"
//#define COMM_SYNTAX_TERM_MSG        "//END//"
#define COMM_SYNTAX_TERM_MSG        "**"
#define COMM_SYNTAX_REQ_LOCONAME    "REQ_LOCO"
#define COMM_SYNTAX_SETCV           "SET_CV"
