#include "stdafx.h"
#include "main.h"

#pragma region
// start
PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
	//sign plugin
	strcpy(outName, "XP-JSON");
	strcpy(outSig, "xp.json");
	strcpy(outDesc, "Sends JSON encoded information via UDP/WebSocket");	

	//read configuration from X-Plane/datarefs.json
	XPLMDebugString("[XP-JSON] Reading config...\n");
	read_config();

	//setup outputs
	XPLMDebugString("[XP-JSON] Setting up Outputs...\n");
	if(enabled_outputs[UDP])
	{
		XPLMDebugString("[XP-JSON] UDP enabled\n");
		winsock_setup();
	}
	if(enabled_outputs[HTTP])
	{
		XPLMDebugString("[XP-JSON] HTTP enabled\n");
		mongoose_setup();
	}
	
	//register flight loop	
	XPLMRegisterFlightLoopCallback(FlightLoopCallback, 5, NULL);

	return 1;
}

// stop
PLUGIN_API void	XPluginStop(void)
{		
	//unregister flight loop
    XPLMUnregisterFlightLoopCallback(FlightLoopCallback, NULL);	

	//close open connections
	if(enabled_outputs[UDP])
	{
		winsock_close();
	}
}


// enable
PLUGIN_API int XPluginEnable(void)
{
    return 1;
}
 
// disable
PLUGIN_API void XPluginDisable(void)
{
}

// recieves message
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage,void* inParam)
{
	int	planeCount;
	switch(inMessage)
	{
		case XPLM_MSG_PLANE_CRASHED:
			sprintf(log_buffer, "[XP-JSON] From System - ID %d, You crashed.\n", inFromWho);
			break;
		case XPLM_MSG_PLANE_LOADED:
			sprintf(log_buffer, "[XP-JSON] From System - ID %d, A new airplane d% has been loaded.\n", inFromWho, inParam);
			break;
		case XPLM_MSG_AIRPORT_LOADED:
			sprintf(log_buffer, "[XP-JSON] From System - ID %d, New airport loaded.\n", inFromWho);
			break;
		case XPLM_MSG_SCENERY_LOADED:
			sprintf(log_buffer, "[XP-JSON] From System - ID %d, Scenary loaded or changed.\n", inFromWho);
			break;
		case XPLM_MSG_AIRPLANE_COUNT_CHANGED:
			XPLMCountAircraft(&planeCount, 0, 0);
			sprintf(log_buffer, "[XP-JSON] From System - ID %d, Airplane count: %d.\n", inFromWho, planeCount);
			break;
		default:
			sprintf(log_buffer, "[XP-JSON] Unknown message %d from %d, param = 0x%08X\n", inMessage, inFromWho, inParam);
			break;
	}
	XPLMDebugString(log_buffer);
}
#pragma endregion PLUGIN_API

#pragma region
void read_config()
{	
	//read the config file
	std::ifstream f("datarefs.json");
	std::string config_str((std::istreambuf_iterator<char>(f)),
    std::istreambuf_iterator<char>());	

	//parse the json and get the config
	rapidjson::Document document;
	document.Parse(config_str.c_str());	
	
	//read server config	
	if(document.FindMember("server")->value.GetType() == rapidjson::kNullType)
	{
		XPLMDebugString("[XP-JSON] server missing in config\n");
		throw std::exception("server missing in config");
	}
	const rapidjson::Value& server = document["server"];
	//udp config (if available)
	if(server.FindMember("udp")->value.GetType() != rapidjson::kNullType)
	{
		const rapidjson::Value& udp = server["udp"];
		if(!udp.HasMember("ip"))
		{			
			XPLMDebugString("[XP-JSON] ip of UDP client missing in config\n");
			throw std::exception("ip of UDP client missing in config");
		}
		udp_ip = std::string(udp["ip"].GetString());
		if(!udp.HasMember("port"))
		{
			XPLMDebugString("[XP-JSON] port of UDP client missing in config\n");
			throw std::exception("port of UDP client missing in config");
		}
		udp_port = udp["port"].GetInt();
		enabled_outputs[UDP] = 1;
	}	
	//http config (if available)
	if(server.FindMember("http")->value.GetType() != rapidjson::kNullType)
	{
		const rapidjson::Value& http = server["http"];
		if(!http.HasMember("port"))
		{			
			XPLMDebugString("[XP-JSON] port of http missing in config\n");
			throw std::exception("port of http missing in config");
		}
		http_port = http["port"].GetInt();
		enabled_outputs[HTTP] = 1;
	}
	
	//read sim config
	if(document.FindMember("sim")->value.GetType() == rapidjson::kNullType)
	{
		XPLMDebugString("[XP-JSON] sim missing in config\n");
		throw std::exception("sim missing in config");
	}
	const rapidjson::Value& sim = document["sim"];
	inInterval = sim["interval"].GetDouble();	
	const rapidjson::Value& drefs = sim["datarefs"];
	for (rapidjson::Value::ConstValueIterator itr = drefs.Begin(); itr != drefs.End(); ++itr)
	{
		datarefs_map[itr->FindMember("name")->value.GetString()] = XPLMFindDataRef(itr->FindMember("dataref")->value.GetString());
	}
}
#pragma endregion CONFIG

#pragma region
static void mongoose_setup()
{
	server = mg_create_server( NULL, ev_handler );
	std::string port = std::to_string( (long long)http_port );
	mg_set_option(server, "listening_port", port.c_str());
	mg_start_thread(serve, server);
}

static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
	if (ev == MG_REQUEST)
	{
		mg_send_header(conn, "Content-Type", "application/json");
		mg_printf_data(conn, datarefs_buffer.GetString());
		return MG_TRUE;
	}
	else if (ev == MG_AUTH)
	{
		return MG_TRUE;
	}
	else
	{
		return MG_FALSE;
	}
}

static void *serve(void *server) {
	for(;;)
	{	  
		mg_poll_server((struct mg_server *) server, 1000);
	}
	return NULL;
}

static void websocket_send() {
	struct mg_connection *c;
	for (c = mg_next(server, NULL); c != NULL; c = mg_next(server, c))
	{
		struct conn_data *d2 = (struct conn_data *) c->connection_param;		
		if (c->is_websocket)
		{
			mg_websocket_printf(c, WEBSOCKET_OPCODE_TEXT, datarefs_buffer.GetString());
		}
    }
}
#pragma endregion MONGOOSE

#pragma region
static void winsock_setup()
{
    //initialise winsock
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        sprintf(log_buffer,"[XP-JSON] Winsock failed. Error Code : %d\n",WSAGetLastError());
		XPLMDebugString(log_buffer);
        exit(EXIT_FAILURE);
    }
     
    //create a udp socket
    if((udp_socket = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET)
    {
		sprintf(log_buffer,"[XP-JSON] Could not create socket : %d\n" , WSAGetLastError());
		XPLMDebugString(log_buffer);
    }
	
	//enable broadcast through udp socket
	int broadcastPermission = 1;
	setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastPermission, sizeof(broadcastPermission));

    //prepare the sockaddr_in structure
	Server.sin_family = AF_INET;
	Server.sin_addr.s_addr = inet_addr(udp_ip.c_str());
    Server.sin_port = htons( udp_port );
}

static void winsock_send()
{	  
	fflush(stdout);
	if (sendto(udp_socket, datarefs_buffer.GetString(), strlen(datarefs_buffer.GetString()), 0, (struct sockaddr*) &Server, sizeof(Server)) == SOCKET_ERROR)
	{
		sprintf(log_buffer,"[XP-JSON] sendto() failed with error code : %d\n" , WSAGetLastError());
		XPLMDebugString(log_buffer);
	}	
}

static void winsock_close()
{
	shutdown(udp_socket, SD_SEND);
	closesocket(udp_socket);
    WSACleanup();
}
#pragma endregion WINSOCK

#pragma region
static void read_datarefs()
{
	datarefs_buffer.Clear();
	rapidjson::Document jdoc;
	jdoc.SetObject();	
	rapidjson::Document::AllocatorType& allocator = jdoc.GetAllocator();

	for (std::map<std::string,XPLMDataRef>::iterator pair=datarefs_map.begin(); pair!=datarefs_map.end(); ++pair)
	{		
		XPLMDataTypeID type = XPLMGetDataRefTypes(pair->second);
		switch(type)
		{
			case xplmType_Int:
				{
					jdoc.AddMember(rapidjson::StringRef(pair->first.c_str()), XPLMGetDatai(pair->second), allocator);
					break;
				}
			case xplmType_Float:
				{	
					jdoc.AddMember(rapidjson::StringRef(pair->first.c_str()), rapidjson::Value().SetDouble(XPLMGetDataf(pair->second)), allocator);
					break;
				}
			case xplmType_Double:
				{
					jdoc.AddMember(rapidjson::StringRef(pair->first.c_str()), rapidjson::Value().SetDouble(XPLMGetDatad(pair->second)), allocator);
					break;
				}
			case 6://other xplmType_Float/Duble?
				{
					jdoc.AddMember(rapidjson::StringRef(pair->first.c_str()), rapidjson::Value().SetDouble(XPLMGetDataf(pair->second)), allocator);
					break;
				}			
			case xplmType_FloatArray:			
				{					
					int size = XPLMGetDatavf(pair->second,NULL,0,0);
					float *values= new float[size];			
					XPLMGetDatavf(pair->second, values, 0, size);
					rapidjson::Value values_array(rapidjson::kArrayType);
					for(int i=0; i<size; i++)
						values_array.PushBack(rapidjson::Value().SetDouble(values[i]), allocator);
					jdoc.AddMember(rapidjson::StringRef(pair->first.c_str()), values_array, allocator);
					delete[] values;
					break;
				}
			case xplmType_IntArray:
				{
					int size = XPLMGetDatavi(pair->second,NULL,0,0);
					int *values= new int[size];			
					XPLMGetDatavi(pair->second, values, 0, size);
					rapidjson::Value values_array(rapidjson::kArrayType);
					for(int i=0; i<size; i++)
						values_array.PushBack(rapidjson::Value().SetInt(values[i]), allocator);
					jdoc.AddMember(rapidjson::StringRef(pair->first.c_str()), values_array, allocator);
					delete[] values;
					break;
				}
			case xplmType_Data:
				{
					int size = XPLMGetDatab(pair->second,NULL,0,0);
					int *values= new int[size];
					char *cvalue= new char[size];
					XPLMGetDatab(pair->second, values, 0, size);
					strcpy(cvalue, (char*) values);
					jdoc.AddMember(rapidjson::StringRef(pair->first.c_str()), rapidjson::StringRef(cvalue), allocator);
					delete[] values;
					break;
				}			
			default: 
				{
					jdoc.AddMember(rapidjson::StringRef(pair->first.c_str()), "unknown dataref type", allocator);
					break;
				}
		}
	}


	SYSTEMTIME tm;
	GetSystemTime(&tm);
	char buf[100];
	sprintf(buf, "%i-%02i-%02iT%02i:%02i:%02i.%03iZ", tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);	
	jdoc.AddMember("timestamp", rapidjson::Value().SetString(buf,allocator), allocator);	
	
	rapidjson::Writer<rapidjson::StringBuffer> writer(datarefs_buffer);
	jdoc.Accept(writer);
}
#pragma endregion DATAREFS SERIALIZATION

#pragma region
float FlightLoopCallback(float elapsedMe, float elapsedSim, int counter, void * refcon)
{
	read_datarefs();

	if(enabled_outputs[HTTP])
	{
		websocket_send();
	}
	if(enabled_outputs[UDP])
	{
		winsock_send();
	}

	return inInterval;
}
#pragma endregion FLIGHT LOOP