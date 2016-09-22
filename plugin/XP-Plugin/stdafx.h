// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS			// Disable deprecation of unsafe functions

//Windows Header Files 
#include <windows.h>
//Other Header Files
#include <string.h>
#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <ctime>
#include <share.h>
//Winsock Library
#include<winsock2.h> 
//#pragma comment(lib,"ws2_32.lib")

/**
 * X-Plane Plugin SDK
 * Allows third party programmers to create plugins for X-Plane.
 * http://www.xsquawkbox.net/xpsdk/index.html
 */
#define IBM	1				//Define Platform for XPLMDefs.h
#include "SDK/CHeaders/XPLM/XPLMMenus.h"
#include "SDK/CHeaders/XPLM/XPLMPlanes.h"
#include "SDK/CHeaders/XPLM/XPLMPlugin.h"
#include "SDK/CHeaders/XPLM/XPLMDisplay.h"
#include "SDK/CHeaders/XPLM/XPLMScenery.h"
#include "SDK/CHeaders/XPLM/XPLMGraphics.h"
#include "SDK/CHeaders/XPLM/XPLMUtilities.h"
#include "SDK/CHeaders/XPLM/XPLMProcessing.h"
#include "SDK/CHeaders/XPLM/XPLMDataAccess.h"
#include "SDK/CHeaders/Widgets/XPWidgets.h"
#include "SDK/CHeaders/Widgets/XPStandardWidgets.h"

/**
 * RapidJSON
 * A fast JSON parser/generator for C++ with both SAX/DOM style API
 * https://github.com/miloyip/rapidjson
 */
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

/**
 * Mongoose
 * Embedded web server for C/C++
 * https://github.com/cesanta/mongoose/
 */
#include "mongoose/mongoose.h"