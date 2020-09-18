#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

typedef struct bgp
{
	const char* bgp_protocol;
	time_t timestamp;
	char routing_protocol;
	const char* peer_ip;
	int peer_asn;
	const char* prefix;
	int aspath[50];
	const char* origin_protocol;
	const char* next_hop;
	int local_pref;
	int med;
	const char* communities;
	const char* atomic_aggregator;
	const char* aggregator;
} bgp;

bgp parsebgp(char* line){
	char* fields[15];
	for(int i=0; i<=13; i++){
		fields[i] = strsep(&line, "|");
	}
	bgp bgpbuff = {};
	bgpbuff.bgp_protocol = fields[0];
	bgpbuff.timestamp = atoi(fields[1]);
	bgpbuff.routing_protocol = *(fields[2]);
	bgpbuff.peer_ip = fields[3];
	bgpbuff.peer_asn = atoi(fields[4]);
	bgpbuff.prefix = fields[5];
	char* aspathstr = fields[6];
	const char* aspathbuff[50];
	for(int i=0; i < 50; i++){
		aspathbuff[i] = strsep(&aspathstr, " ");
		if(!aspathbuff[i]){
			bgpbuff.aspath[i] = 0;
			break;
		}
		bgpbuff.aspath[i] = atoi(aspathbuff[i]);
	}
	bgpbuff.origin_protocol = fields[7];
	bgpbuff.next_hop = fields[8];
	bgpbuff.local_pref = atoi(fields[9]);
	bgpbuff.med = atoi(fields[10]);
	bgpbuff.communities = fields[11];
	bgpbuff.atomic_aggregator = fields[12];
	bgpbuff.aggregator = fields[13];
	return bgpbuff;
}

bool asselect(const int* aspath, const int majoras, const int* excludeas, const int excludeassize){
	bool ascontain = false;
	bool asexclude = false;
	int asindex = -1;
	for(int i=0; aspath[i] != 0; i++){
		if(aspath[i] == majoras){
			ascontain = true;
			asindex = i;
			break;
		}
	}
/*
 * For example, AS4837 (China Unicom) may also announce AS4538 (CERNET)
 * to AS2497 (IIJ). If AS4538 is excluded, it should not be regarded to
 * be routed by AS4837. Conversely, being announced AS4837 does not affect
 * AS4538 to be the upstream of AS24362.
 *
 * Of course, there should be some special cases. For example, some ASs
 * may privately peer with AS4134 (China Telecom) and only announced in
 * China and other global routes may be announced by another AS. However,
 * this is so special that ignoring it does not matter.
 */
	if(ascontain){
		for(int i = asindex+1; aspath[i] != 0; i++){
			for(int j=0; j<=excludeassize-1; j++){
				if(aspath[i] == excludeas[j]){
					asexclude = true;
					break;
				}
			}
			if(asexclude){
				break;
			}
		}
	}
	if(ascontain && !asexclude){
		return true;
	}else{
		return false;
	}

}

int main(const int argc, const char* argv[]){
	int majoras = atoi(argv[1]);
	int* excludeas = calloc(argc-2, sizeof(int));
	for(int i=2; i <= argc-1; i++){
		excludeas[i-2] = atoi(argv[i]);
	}

	char* line = NULL;
	size_t linelength = 0;
	while (getline(&line, &linelength, stdin) != -1){
		const bgp route = parsebgp(line);
		bool routeselected = asselect(route.aspath, majoras, excludeas, argc-2);
		if(routeselected){
			printf("%s\n", route.prefix);
		}
	}
	free(excludeas);
}
