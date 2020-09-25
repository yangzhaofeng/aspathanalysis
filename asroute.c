#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

const int tier1[] = {
	7018, // AT&T
	3356, // Level 3
//	3549, // Level 3
	3320, // Deuteche Telekom
	3257, // GTT
	6830, // liberty global
	2914, // NTT
	5511, // Orange
	3491, // PCCW
	1239, // Sprint
	6453, // TATA
	6762, // TELECOM ITALIA
	12956, // Telefonica
	1299, // Telia
	701, // Verizon UUnet
	6461, // Zayo
	174, // Cogent
	2828, // MCI Verizon Business
	6939, // Hurricane Electric
	7922 // Comcast
};

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

int aslength(const int* aspath, const int majoras){
	int pathlength = 0;
	int length = -1;
	for(int i=0; aspath[i] != 0; i++){
		pathlength++;
	}
	//fprintf(stderr, "pathlength = %d\n", pathlength);
	int asindex = pathlength - 1;
	int lastas = aspath[pathlength-1];
	for(int i = pathlength-1; i >= 0; i--){
		if(aspath[i] == majoras){
			length = asindex - i;
			break;
		}else{
			if(aspath[i] == lastas){
				asindex = i; // ignore AS suspend
			}
		}
	}
	return length;
}

bool tier1contain(const int* aspath){
	bool contain = false;
	for(int i=0; i<sizeof(tier1)/sizeof(int); i++){
		if(asselect(aspath, tier1[i], NULL, 0)){
			contain = true;
			break;
		}
	}
	return contain;
}

int tier1length(const int* aspath){
	int length = -1;
	//fprintf(stderr, "sizeof(tier1)/sizeof(int) = %d\n", sizeof(tier1)/sizeof(int));
	for(int i=0; i<sizeof(tier1)/sizeof(int); i++){
		int lengthbuff = aslength(aspath, tier1[i]);
		if(lengthbuff != -1 && (length == -1 || lengthbuff < length)){
			length = lengthbuff;
		}
	}
	return length;
}

int main(const int argc, const char* argv[]){
	//fprintf(stderr, "sizeof(tier1)/sizeof(int) = %d\n", sizeof(tier1)/sizeof(int));
	int majoras = atoi(argv[1]);
	int* excludeas = calloc(argc-2, sizeof(int));
	for(int i=2; i <= argc-1; i++){
		excludeas[i-2] = atoi(argv[i]);
	}

	char* line = NULL;
	size_t linelength = 0;
	char prevprefix[44]; // 0000:0000:0000:0000:0000:0000:0000:0000/128
	int length_majoras = -1;
	int length_tier1 = -1;
	//bool tomajoras = false;
	//bool tootheras = false;
	//bool ismajoras = false;
	while (getline(&line, &linelength, stdin) != -1){
		if(line[0] != 'T'){
			continue;
		}
		const bgp route = parsebgp(line);
		if(strcmp(prevprefix, route.prefix) != 0){
			//fprintf(stderr, "prefix == %s, length_majoras == %d, length_tier1 == %d\n", prevprefix, length_majoras, length_tier1);
			if( (length_majoras != -1 && (length_tier1 == -1 || (length_majoras == 0 && length_tier1 == 0) || length_majoras < length_tier1))){
				//fprintf(stderr, "length_majoras == %d, length_tier1 == %d\n", length_majoras, length_tier1);
				printf("%s\n", prevprefix);
			}
			length_majoras = -1;
			length_tier1 = -1;
			//tomajoras = false;
			//tootheras = false;
			prevprefix[0] = 0;
		}
		bool majorselect = asselect(route.aspath, majoras, excludeas, argc-2);
		bool norighttier1 = asselect(route.aspath, majoras, tier1, sizeof(tier1)/sizeof(int));
		bool routeselected = majorselect && norighttier1;
		if(routeselected){
			//tomajoras = true;
			int length = aslength(route.aspath, majoras);
			bool intier1 = tier1contain(route.aspath);
			if(!(intier1 || length == 0)){
				length = -1;
			}
			if(length!=-1 && (length_majoras==-1 || length<length_majoras)){
				length_majoras = length;
			}
		}else{
			//tootheras = true;
			int length = tier1length(route.aspath);
			if(length!=-1 && (length_tier1==-1 || length<length_tier1)){
				length_tier1 = length;
			}
		}
		strcpy(prevprefix, route.prefix);
	}
	if( (length_majoras != -1 && (length_tier1 == -1 || (length_majoras == 0 && length_tier1 == 0) || length_majoras < length_tier1))){
		//fprintf(stderr, "length_majoras == %d, length_tier1 == %d\n", length_majoras, length_tier1);
		printf("%s\n", prevprefix);
	}
	length_majoras = -1;
	length_tier1 = -1;
	prevprefix[0] = 0;

	free(excludeas);
}
