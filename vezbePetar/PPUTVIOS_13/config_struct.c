#include "config_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
void config_init(struct MyConfigStruct *config)
{
    int i = 0;
    FILE *file = NULL;
    char chunk[30];
 
    file = fopen("config.ini", "r");
    if (file == NULL)
    {
        printf("File can't be opened \n");
        return;
    }
 
    printf("Config file opened successfully!\n");
 
    while (fgets(chunk, sizeof(chunk), file) != NULL)
    {
        remove_spaces(chunk);
        char string_to_compare[30]= "";
        char value[15] = "";
        int position;
 
        for(i = 0; chunk[i] != '\0'; i++)
        {
            position = 0;
            if (chunk[i] == '=')
            {
                position = i;
                strncpy(string_to_compare, chunk, position);
                printf("String to compare: %s\n", string_to_compare);
                strncpy(value, chunk+position+1, strlen(chunk)-position-1);
                value[strlen(value)-1] = '\0';
                printf("Value: %s\n", value);
                set_config_structure(config,string_to_compare, value);
            }
        }
 
    }
 
    print_config_structure(config);
 
    fclose(file);
    printf("Conifg file closed successfully\n");
}
 
void remove_spaces(char *s) {
    char *d = s;
    do {
        while (*d == ' ' || *d == ';') {
            ++d;
        }
    } while (*s++ = *d++);
}
 
void set_config_structure(struct MyConfigStruct *config, char *string, char *value)
{
    if(strcmp(string, "frequency") == 0)
    {
        config->frequency = atoi(value);
    }
    else if(strcmp(string, "bandwidth") == 0)
    {
        config->bandwidth = atoi(value);
    }
    else if(strcmp(string, "module") == 0)
    {
        strncpy(config->module, value, sizeof(config->module));
    }
    else if(strcmp(string, "apid") == 0)
    {
        config->init_channel.audio_pid = atoi(value);
    }
    else if(strcmp(string, "vpid") == 0)
    {
        config->init_channel.video_pid = atoi(value);
    }
    else if(strcmp(string, "atype") == 0)
    {
        strncpy(config->init_channel.audio_type, value, sizeof(config->init_channel.audio_type));
    }
    else if(strcmp(string, "vtype") == 0)
    {
        strncpy(config->init_channel.video_type, value, sizeof(config->init_channel.video_type));
    }
}
 
void print_config_structure(struct MyConfigStruct *config)
{
    printf("Frequency: %d\n", config->frequency);
    printf("Bandwitdh: %d\n", config->bandwidth);
    printf("Module: %s\n", config->module);
    printf("Audio PID: %d\n", config->init_channel.audio_pid);
    printf("Video PID: %d\n", config->init_channel.video_pid);
    printf("Audio Type: %s\n", config->init_channel.audio_type);
    printf("Video Type: %s\n", config->init_channel.video_type);
}

