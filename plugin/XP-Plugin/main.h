#pragma once 
#define UDP 0
#define HTTP 1
static int enabled_outputs[2]= {0,0};

static char log_buffer[255]= {'\0'};

//HTTP server
#pragma region
static int http_port;
struct mg_server *server;
static void mongoose_setup();
static int ev_handler(struct mg_connection *conn, enum mg_event ev);
static void *serve(void *server);
static void websocket_send();
#pragma endregion MONGOOSE 

//UDP socket
#pragma region
static std::string udp_ip;
static int udp_port;
static SOCKET udp_socket;
static struct sockaddr_in Server;
static WSADATA wsa;
static void winsock_setup();
static void winsock_send();
static void winsock_close();
#pragma endregion WINSOCK UDP

//datarefs
#pragma region
static float inInterval = 0.1f;
rapidjson::StringBuffer datarefs_buffer;
static std::map<std::string, XPLMDataRef> datarefs_map;
static void read_config();
static void read_datarefs();
#pragma endregion XPLMDataRefs

//flight loop
float FlightLoopCallback(float elapsedMe, float elapsedSim, int counter, void * refcon);