#include "tdp_api.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DESIRED_FREQUENCY 818000000	/* Tune frequency in Hz */
#define BANDWIDTH 8    				/* Bandwidth in Mhz */
#define VIDEO_PID 101				/* Channel video pid */
#define AUDIO_PID 103				/* Channel audio pid */
#define VIDEO_TYPE_MPEG2 42
#define AUDIO_TYPE_MPEG_AUDIO 10


static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t tunerStatusCallback(t_LockStatus status);
int32_t mySecFilterCallback(uint8_t *buffer);
int32_t parsePAT(uint8_t *buffer);
int32_t parsePMT(uint8_t *buffer);

struct PatTable
{
    uint16_t section_length;
    uint16_t transport_stream_id;
    uint8_t version_number;

};

struct Stream
{
    uint8_t stream_type;
    uint16_t elementary_pid;
    uint16_t ES_info_length;
};

struct PmtTable
{
    uint16_t section_length;
    uint16_t program_number;
    uint16_t program_info_lenght;
    uint8_t stream_count;
    struct Stream streamArray[16];

};

struct ChannelInfo{
    uint16_t program_number;
    uint16_t pid;
};

struct PatTable     patTable;
struct PmtTable     pmtTable;
struct PmtTable     pmtTableArray[16];
struct Stream       stream;
struct ChannelInfo  channelArray[8];
int pmtTableCount = 0;


uint16_t getVideoElementaryPID(struct PmtTable pmtTable);
uint16_t getAudioElementaryPID(struct PmtTable pmtTable);


int main()
{    
    int32_t result;

    uint32_t playerHandle = 0;
    uint32_t sourceHandle = 0;
    uint32_t filterHandle = 0;
    
    uint32_t audioStreamHandle = 0;
    uint32_t videoStreamHandle = 0;
    
	struct timespec lockStatusWaitTime;
	struct timeval now;
    
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;
       
    /*Initialize tuner device*/
    if(Tuner_Init())
    {
        printf("\n%s : ERROR Tuner_Init() fail\n", __FUNCTION__);
        return -1;
    }
    
    /* Register tuner status callback */
    if(Tuner_Register_Status_Callback(tunerStatusCallback))
    {
		printf("\n%s : ERROR Tuner_Register_Status_Callback() fail\n", __FUNCTION__);
	}
    
    /*Lock to frequency*/
    if(!Tuner_Lock_To_Frequency(DESIRED_FREQUENCY, BANDWIDTH, DVB_T))
    {
        printf("\n%s: INFO Tuner_Lock_To_Frequency(): %d Hz - success!\n",__FUNCTION__,DESIRED_FREQUENCY);
    }
    else
    {
        printf("\n%s: ERROR Tuner_Lock_To_Frequency(): %d Hz - fail!\n",__FUNCTION__,DESIRED_FREQUENCY);
        Tuner_Deinit();
        return -1;
    }
    
    /* Wait for tuner to lock*/
    pthread_mutex_lock(&statusMutex);
    if(ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("\n%s:ERROR Lock timeout exceeded!\n",__FUNCTION__);
        Tuner_Deinit();
        return -1;
    }
    pthread_mutex_unlock(&statusMutex);
   
    /**TO DO:**/
    /*Initialize player, set PAT pid to demultiplexer and register section filter callback*/
    Player_Init(&playerHandle);
          /* Open source (open data flow between tuner and demux) */
    Player_Source_Open(playerHandle, &sourceHandle);
 

    
    /* Register section filter callback */
    Demux_Register_Section_Filter_Callback(mySecFilterCallback);
    
    /* Set filter to demux */
    Demux_Set_Filter(playerHandle, 0x0000, 0x00, &filterHandle);
    sleep(1);
    int i;

    for(i=1;i<8;i++){
        Demux_Free_Filter(playerHandle, filterHandle);
        Demux_Set_Filter(playerHandle, channelArray[i].pid, 0x02, &filterHandle);
        sleep(2);
    };
    Demux_Free_Filter(playerHandle, filterHandle);
    Demux_Unregister_Section_Filter_Callback(mySecFilterCallback);
    printf("++EOParsing++");


        
    /**TO DO:**/
    /*Play audio and video*/
    //Player_Stream_Create(uint32_t playerHandle, uint32_t sourceHandle, uint32_t PID, tStreamType streamType, uint32_t *streamHandle);
    Player_Stream_Create(playerHandle,sourceHandle,1001,VIDEO_TYPE_MPEG2,&videoStreamHandle);
    Player_Stream_Create(playerHandle,sourceHandle,1003,AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
    
	/* Wait for a while */
    fflush(stdin);
    getchar();
    
    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);

    Player_Stream_Create(playerHandle,sourceHandle,getVideoElementaryPID(pmtTableArray[5]),VIDEO_TYPE_MPEG2,&videoStreamHandle);
    Player_Stream_Create(playerHandle,sourceHandle,getAudioElementaryPID(pmtTableArray[5]),AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
    
    fflush(stdin);
    getchar(); 

    /**TO DO:**/
    /*Deinitialization*/
    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
    
    Player_Source_Close(playerHandle, sourceHandle);
    Player_Deinit(playerHandle);
    
    
    /*Deinitialize tuner device*/
    Tuner_Deinit();
  
    return 0;
}

int32_t tunerStatusCallback(t_LockStatus status)
{
    if(status == STATUS_LOCKED)
    {
        pthread_mutex_lock(&statusMutex);
        pthread_cond_signal(&statusCondition);
        pthread_mutex_unlock(&statusMutex);
        printf("\n%s -----TUNER LOCKED-----\n",__FUNCTION__);
    }
    else
    {
        printf("\n%s -----TUNER NOT LOCKED-----\n",__FUNCTION__);
    }
    return 0;
}

int32_t mySecFilterCallback(uint8_t *buffer)
{   
    uint16_t table_type = *(buffer);
    if(table_type==0x00) parsePAT(buffer);
    if(table_type==0x02) parsePMT(buffer);
    return 0;
}

int32_t parsePAT(uint8_t *buffer){
    printf("****PAT****\n");
    
    patTable.section_length = (uint16_t)(((*(buffer+1)<<8) + *(buffer + 2)) & 0x0FFF);
    //printf("Length:%d ", patTable.section_length);

    patTable.transport_stream_id = (uint16_t)(((*(buffer+3)<<8) + *(buffer+4)));
    //printf("Transport stream ID:%d ", patTable.transport_stream_id);

    patTable.version_number = (uint8_t)((*(buffer+5)>>1 & 0x1F));
    //printf("Version Number:%d ", patTable.version_number);

    int broj_kanala = (patTable.section_length*8-64)/32;
    //printf("Broj Kanala :%d \n",broj_kanala);
    
    int i;
    for(i = 0; i<broj_kanala;i++){
        channelArray[i].program_number = (uint16_t)(*(buffer+8+(4*i))<<8)+(*(buffer+9+(4*i)));
        channelArray[i].pid = (uint16_t)((*(buffer+10+(i*4))<<8) + (*(buffer+11+(i*4))) & 0x1FFF);

        //printf("i : %d | p# : %d | pid : %d \n",i,channelArray[i].program_number,channelArray[i].pid);
    }
    
    printf("****-EOPAT****\n");
    return 0;
}

int32_t parsePMT(uint8_t *buffer){

    pmtTable.program_number = (uint16_t) ((*(buffer+3)<<8)  + *(buffer+4));
       printf("***************PMT - %d**************\n",pmtTable.program_number);

    pmtTable.section_length = (uint16_t)(((*(buffer+1)<<8) + *(buffer + 2)) & 0x0FFF);
    printf("    Length: %d ", pmtTable.section_length);
    pmtTable.program_number = (uint16_t) ((*(buffer+3)<<8)  + *(buffer+4));
    printf("Program number: %d ", pmtTable.program_number);
    pmtTable.program_info_lenght = (uint16_t) (((*(buffer+10)<<8) + *(buffer+11)) & 0x0FFF);
    printf("Program Info Len : %d ", pmtTable.program_info_lenght);
    pmtTable.stream_count = (uint8_t)(pmtTable.section_length - (12 + pmtTable.program_info_lenght + 4 - 3)) / 8;
    printf("Stream Count: %d \n", pmtTable.stream_count);

    int i;
    uint8_t *m_buffer = (uint8_t *)(buffer) + 12 + pmtTable.program_info_lenght;
    
    for(i=0; i<pmtTable.stream_count;i++){

        stream.stream_type = m_buffer[0];
        stream.elementary_pid = (uint16_t) ((*(m_buffer+1)<<8) + *(m_buffer + 2)) & 0x1FFF;
        stream.ES_info_length = (uint16_t) ((*(m_buffer+3)<<8) + *(m_buffer + 4)) & 0x0FFF;
        printf("    i: %d  |  stream_type:%d, elementary_pid:%d, ES_info:%d \n",i,stream.stream_type, stream.elementary_pid, stream.ES_info_length);
        m_buffer += 5 + stream.ES_info_length;

        pmtTable.streamArray[i] = stream;
    }
    
    pmtTableArray[pmtTableCount] = pmtTable;
    pmtTableCount++;

    printf("*********************************************\n");
    
    return 0;
}

uint16_t getVideoElementaryPID(struct PmtTable pmtTable){
    int i;
    for(i=0;i<pmtTable.stream_count;i++)
    {
        if(pmtTable.streamArray[i].stream_type==2){ return pmtTable.streamArray[i].elementary_pid; }
    }

    printf("Error: VideoPID");
    return 1;
};

uint16_t getAudioElementaryPID(struct PmtTable pmtTable){
    int i;
    for(i=0;i<pmtTable.stream_count;i++)
    {
        if(pmtTable.streamArray[i].stream_type==3){ return pmtTable.streamArray[i].elementary_pid; }
    }

    printf("Error: AudioPID");
    return 1;
};