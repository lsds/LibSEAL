
#include <string.h>
#include <stdio.h>
#include "jsmn.h"

char *js = 
"{\"command\":\"sync_ops\",\"args\":{\"es_id\":\"34ycf89oph8n1h7y3dy19b9ensvscs\",\"member_id\":\"1\",\"seq_head\":\"134\",\"client_ops\":5}}";

//"{\"command\":\"sync_ops\",\"args\":{\"es_id\":\"34ycf89oph8n1h7y3dy19b9ensvscs\",\"member_id\":\"1\",\"seq_head\":\"134\",\"client_ops\":[{\"optype\":\"MoveCursor\",\"memberid\":\"1\",\"timestamp\":1468569686720,\"position\":2119,\"length\":0,\"selectionType\":\"Range\"},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569687622,\"position\":2118,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688109,\"position\":2117,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688137,\"position\":2116,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688170,\"position\":2115,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688203,\"position\":2114,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688238,\"position\":2113,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688273,\"position\":2112,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688299,\"position\":2111,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688324,\"position\":2110,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688352,\"position\":2109,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688443,\"position\":2108,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688471,\"position\":2107,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688503,\"position\":2106,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688533,\"position\":2105,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688562,\"position\":2104,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688593,\"position\":2103,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688622,\"position\":2102,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688653,\"position\":2101,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688684,\"position\":2100,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688714,\"position\":2099,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688812,\"position\":2098,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688836,\"position\":2097,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688854,\"position\":2096,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688872,\"position\":2095,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688898,\"position\":2094,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688934,\"position\":2093,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688962,\"position\":2092,\"length\":1},{\"optype\":\"RemoveText\",\"memberid\":\"1\",\"timestamp\":1468569688989,\"position\":2091,\"length\":1}]}}";


//char *js="{ \"name\" : \"Jack\", \"age\" : 27 }";


int main() {
	jsmn_parser parser;


	// js - pointer to JSON string
	// tokens - an array of tokens available
	// 10 - number of tokens available

	jsmn_init(&parser);
	int toparse = jsmn_parse(&parser, js, strlen(js), NULL, 0);
	printf("read: %d\n",toparse);

	jsmn_init(&parser);
	jsmntok_t tokens[toparse];
	int cnt = jsmn_parse(&parser, js, strlen(js), tokens, toparse);
	printf("read: %d\n",cnt);
}
