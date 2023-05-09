#ifndef __INI_PARSER_H__
#define __INI_PARSER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "tdp_api.h"

typedef struct Config{
    int frequency;
    int bandwidth;
    char* module;
    int aPid;
    int vPid;
    char satype[4];
    int aType;
    char svtype[5];
    int vType;
    char time[5];
    int channel_index;
} Config;

void parseConfigXML(char *filename, struct Config *config);


#endif
