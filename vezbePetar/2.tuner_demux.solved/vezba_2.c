#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "errno.h"
#include "tdp_api.h"
#include <string.h>




static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
   char command[13];

   /* Command is the control command to the terminal */
   sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
   printf("%s", command);
}

#define CONFIG_FILE "config.ini"

#define ASSERT_TDP_RESULT(x,y)  if(NO_ERROR == x) \
                                    printf("%s success\n", y); \
                                else{ \
                                    textColor(1,1,0); \
                                    printf("%s fail\n", y); \
                                    textColor(0,7,0); \
                                    return -1; \
                                }

int32_t myPrivateTunerStatusCallback(t_LockStatus status);
int32_t mySecFilterCallback(uint8_t *buffer);
pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

struct Config{
    int bandwith;
    int frequency;
};

struct PatTable
{
    uint16_t section_length;
    uint16_t transport_stream_id;
    uint8_t version_number;
    
};

struct PatTable patTable;

void parse(char *filename, struct Config *config){

    int frequency = 0, bandwith = 0, has_frequency = 0, has_bandwith = 0;
    char buf[800];
    FILE *file;

    file = fopen(filename,"r");

    while(fgets(buf,sizeof buf, file))
    {
        if(buf[strspn(buf, " ")] == '\n')
            continue;

        if(sscanf(buf, "frequency = %d", &config->frequency) == 1)
            has_frequency = 1;
        else
        if (sscanf(buf, "bandwith = %d", &config->bandwith) == 1)
            has_bandwith = 1;
        else
            fprintf(stderr, "invalid line: %s", buf);
    }
    fclose(file);

    return;

}


int32_t main()
{
    
    struct Config config;
    parse(CONFIG_FILE, &config);
    int32_t result;
    
    uint32_t playerHandle = 0;
    uint32_t sourceHandle = 0;
    uint32_t filterHandle = 0;
    
    struct timespec lockStatusWaitTime;
    struct timeval now;
    
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;
    
    /* Initialize tuner */
    result = Tuner_Init();
    ASSERT_TDP_RESULT(result, "Tuner_Init");
    
    /* Register tuner status callback */
    result = Tuner_Register_Status_Callback(myPrivateTunerStatusCallback);
    ASSERT_TDP_RESULT(result, "Tuner_Register_Status_Callback");
    
    /* Lock to frequency */
    result = Tuner_Lock_To_Frequency(config.frequency, config.bandwith, DVB_T);
    ASSERT_TDP_RESULT(result, "Tuner_Lock_To_Frequency");
    
    pthread_mutex_lock(&statusMutex);
    if(ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("\n\nLock timeout exceeded!\n\n");
        return -1;
    }
    pthread_mutex_unlock(&statusMutex);
    
    /* Initialize player (demux is a part of player) */
    result = Player_Init(&playerHandle);
    ASSERT_TDP_RESULT(result, "Player_Init");
    
    /* Open source (open data flow between tuner and demux) */
    result = Player_Source_Open(playerHandle, &sourceHandle);
    ASSERT_TDP_RESULT(result, "Player_Source_Open");
    
    /* Set filter to demux */
    result = Demux_Set_Filter(playerHandle, 0x0000, 0x00, &filterHandle);
    ASSERT_TDP_RESULT(result, "Demux_Set_Filter");
    
    /* Register section filter callback */
    result = Demux_Register_Section_Filter_Callback(mySecFilterCallback);
    ASSERT_TDP_RESULT(result, "Demux_Register_Section_Filter_Callback");
    
    /* Wait for a while to receive several PAT sections */
    fflush(stdin);
    getchar();
    
    /* Deinitialization */
    
    /* Free demux filter */
    result = Demux_Free_Filter(playerHandle, filterHandle);
    ASSERT_TDP_RESULT(result, "Demux_Free_Filter");
    
    /* Close previously opened source */
    result = Player_Source_Close(playerHandle, sourceHandle);
    ASSERT_TDP_RESULT(result, "Player_Source_Close");
    
    /* Deinit player */
    result = Player_Deinit(playerHandle);
    ASSERT_TDP_RESULT(result, "Player_Deinit");
    
    /* Deinit tuner */
    result = Tuner_Deinit();
    ASSERT_TDP_RESULT(result, "Tuner_Deinit");
    
    return 0;
}

int32_t myPrivateTunerStatusCallback(t_LockStatus status)
{
    if(status == STATUS_LOCKED)
    {
        pthread_mutex_lock(&statusMutex);
        pthread_cond_signal(&statusCondition);
        pthread_mutex_unlock(&statusMutex);
        printf("\n\n\tCALLBACK LOCKED\n\n");
    }
    else
    {
        printf("\n\n\tCALLBACK NOT LOCKED\n\n");
    }
    return 0;
}

int32_t mySecFilterCallback(uint8_t *buffer)
{
    printf("\n\nSection arrived!!!\n\n");
    
    patTable.section_length = (uint16_t)(((*(buffer+1)<<8) + *(buffer + 2)) & 0x0FFF);
    printf("Length: %d ", patTable.section_length);

    patTable.transport_stream_id = (uint16_t)(((*(buffer+3)<<8) + *(buffer+4)));
    printf("Transport stream ID: %d ", patTable.transport_stream_id);

    patTable.version_number = (uint8_t)((*(buffer+5)>>1 & 0x1F));
    printf("Version Number: %d ", patTable.version_number);

    printf("Broj Kanala : %d \n",(patTable.section_length*8-64)/32);
    return 0;
}
