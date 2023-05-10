#ifndef CONFIG_STRUCT
#define CONFIG_STRUCT
 
struct initial_channel{
    int audio_pid;
    int video_pid;
    char audio_type[10];
    char video_type[10];
};
 
struct MyConfigStruct{
    int frequency;
    int bandwidth;
    char module[10];
    struct initial_channel init_channel;
};
 
void config_init(struct MyConfigStruct *config);
void remove_spaces(char *s);
void set_config_structure(struct MyConfigStruct *config, char *string, char *value);
void print_config_structure(struct MyConfigStruct *config);


#endif