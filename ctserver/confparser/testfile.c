#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "confparser.h"


#define COMMAND_MAX 1024
int
main(int argc, char **argv)
{
	int timestamp=0;
	char command[COMMAND_MAX + 1];
	char *conf_file = argv[1];

	struct conf_str_config conf_str_array[] = {
		{"command", command}, 
		{0, 0}
	};
	struct conf_int_config conf_int_array[] = {
		{"timestamp", &timestamp},
		{0,0} 
	};
	dictionary *dict = open_conf_file(conf_file);
	if (dict == NULL) {
		fprintf(stderr,"errror");
	}
	parse_conf_file(dict, "Section", conf_int_array, conf_str_array);
	close_conf_file(dict);
	printf("%s\n%d\n", command,timestamp);
}

