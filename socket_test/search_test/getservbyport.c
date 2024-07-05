#include "netdb.h"
#include "stdio.h"
int main() {
	    struct servent *se = NULL;
	    int i = 0;	 
	    se = getservbyport(htons(21), "tcp");
	    if (!se)
	         return -1;			
	    printf("name : %s\n", se->s_name);
	    printf("port : %d\n", ntohs(se->s_port));
	    printf("proto : %s\n", se->s_proto);
	    for (i = 0; se->s_aliases[i]; i++)
	         printf("aliases : %s\n", se->s_aliases[i]); 
	    return 0;
}
