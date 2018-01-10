#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEVICEFILE	"/tmp/device_tables/local_device_table"
#define MAXDEVICE	512
#define BUFSIZE		512
#define MACSIZE		32
#define TYPESIZE	32
#define MODELSIZE	32
#define NAMESIZE	32

typedef struct device_info {
	char mac[MACSIZE];
	char type[TYPESIZE];
	char model[MODELSIZE];
	char name[NAMESIZE];
	unsigned int flag;
} DeviceInfo;

enum {
	CUSTOMIZE,
	UPDATE
};

int main(int argc, char ** argv) {
	FILE *fp;
	DeviceInfo newInfo, matchInfo;
	DeviceInfo Info[MAXDEVICE];
	char line[BUFSIZE], *newline, *tmpmac;
	int i, count, match = 0, type;

	if (argc == 6 && (strcmp(argv[1], "customizename") == 0 || strcmp(argv[1], "customize") == 0)) {
		type = CUSTOMIZE;
		strncpy(newInfo.mac, argv[2], MACSIZE);
		strncpy(newInfo.type, argv[3], TYPESIZE);
		strncpy(newInfo.model, argv[4], MODELSIZE);
		strncpy(newInfo.name, argv[5], NAMESIZE);
		newInfo.flag = 1;
	} else if(argc == 5 && strcmp(argv[1], "update") == 0) {
		type = UPDATE;
		strncpy(newInfo.mac, argv[2], MACSIZE);
		strncpy(newInfo.type, argv[3], TYPESIZE);
		strncpy(newInfo.model, argv[4], MODELSIZE);
		strcpy(newInfo.name, "Unkonwn");
		newInfo.flag = 0;
	} else {
		fprintf(stderr, "Usage:%s {customizename mac type model name|update mac type model}\n", argv[0]);
		return -1;
	}

	fp = fopen(DEVICEFILE, "r");
	if (fp == NULL) { fprintf(stderr, "[%s][%d]Failed to open %s\n", __FILE__, __LINE__, DEVICEFILE); return -1;}

	for (i = 0; fgets(line, sizeof(line), fp) && i < MAXDEVICE; i++) {
		newline = strtok(line, "\n");
		tmpmac = strtok(newline, ",");
		if (match == 1 || strcmp(tmpmac, newInfo.mac) != 0) {
			strncpy(Info[i].mac, tmpmac, MACSIZE);
			strncpy(Info[i].type, strtok(NULL, ","), TYPESIZE);
			strncpy(Info[i].model, strtok(NULL, ","), MODELSIZE);
			strncpy(Info[i].name, strtok(NULL, ","), NAMESIZE);
			Info[i].flag = atoi(strtok(NULL, ","));
		} else {
			i--;
			match = 1;
			strncpy(matchInfo.mac, tmpmac, MACSIZE);
			strncpy(matchInfo.type, strtok(NULL, ","), TYPESIZE);
			strncpy(matchInfo.model, strtok(NULL, ","), MODELSIZE);
			strncpy(matchInfo.name, strtok(NULL, ","), NAMESIZE);
			matchInfo.flag = atoi(strtok(NULL, ","));
		}
	}
	fclose(fp);

	if (i == MAXDEVICE) {
		count = i;
		strncpy(Info[0].mac, newInfo.mac, MACSIZE);
		strncpy(Info[0].type, newInfo.type, TYPESIZE);
		strncpy(Info[0].model, newInfo.model, MODELSIZE);
		strncpy(Info[0].name, newInfo.name, NAMESIZE);
		Info[0].flag = newInfo.flag;
	} else {
		count = i + 1;
		if (type == CUSTOMIZE || match == 0 || (match == 1 && type == UPDATE && matchInfo.flag == 0)) {
			strncpy(Info[i].mac, newInfo.mac, MACSIZE);
			strncpy(Info[i].type, newInfo.type, TYPESIZE);
			strncpy(Info[i].model, newInfo.model, MODELSIZE);
			strncpy(Info[i].name, newInfo.name, NAMESIZE);
			Info[i].flag = newInfo.flag;
		} else {
			strncpy(Info[i].mac, matchInfo.mac, MACSIZE);
			strncpy(Info[i].type, matchInfo.type, TYPESIZE);
			strncpy(Info[i].model, matchInfo.model, MODELSIZE);
			strncpy(Info[i].name, matchInfo.name, NAMESIZE);
			Info[i].flag = matchInfo.flag;
		}
	}

	fp = fopen(DEVICEFILE, "w");
	if (fp == NULL) { fprintf(stderr, "[%s][%d]Failed to open %s\n", __FILE__, __LINE__, DEVICEFILE); return -1;}

	for (i = 0; i < count; i++) 
		fprintf(fp, "%s,%s,%s,%s,%d\n", Info[i].mac, Info[i].type, Info[i].model, Info[i].name, Info[i].flag);

	fclose(fp);

	return 0;
}
