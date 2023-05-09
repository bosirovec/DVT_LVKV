#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "errno.h"
#include "tdp_api.h"
#include "xml_parser.h"
#include <string.h>
#include <stdint.h>
#include <directfb.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>



#define NUM_EVENTS  5
#define CONFIG_FILE "config.xml"
#define VIDEO_TYPE_MPEG2 42
#define AUDIO_TYPE_MPEG_AUDIO 10
#define DESIRED_FREQUENCY 818000000	/* Tune frequency in Hz */
#define BANDWIDTH 8
#define VIDEO_PID 101				/* Channel video pid */
#define AUDIO_PID 103

static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t tunerStatusCallback(t_LockStatus status);
int32_t mySecFilterCallback(uint8_t *buffer);
int32_t parsePAT(uint8_t *buffer);
int32_t parsePMT(uint8_t *buffer);
int32_t remoteMainFunction();
int32_t getKey(int32_t count, uint8_t* buf, int32_t* eventsRead);
static void *remoteThreadTask();
//int32_t drawPressedButton();

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

typedef struct{
    uint16_t section_length;
    uint16_t program_info_lenght;
    uint16_t program_number;
    uint16_t stream_type[15];
    uint16_t elemetary_pid[15];
    uint16_t ES_info_lenght[15];
}Pmt;
Pmt pmt[16]; 
int pmt_count = 0;
 

struct PatTable     patTable;
struct PmtTable     pmtTable;
struct PmtTable     pmtTableArray[16];
struct Stream       stream;
struct ChannelInfo  channelArray[8];
int pmtTableCount = 0;
Config config;
static pthread_t remote;
int main_running = 1;
static int32_t inputFileDesc;
int channel = 0;
int volume = 50;

uint16_t getVideoElementaryPID(struct PmtTable pmtTable);
uint16_t getAudioElementaryPID(struct PmtTable pmtTable);

uint32_t playerHandle = 0;
uint32_t sourceHandle = 0;
uint32_t filterHandle = 0;
    
uint32_t audioStreamHandle = 0;
uint32_t videoStreamHandle = 0;

bool valueinarray(uint16_t val, uint16_t arr[], size_t n);
uint16_t videoPidArray[7] = {0,0,0,0,0,0,0};
uint16_t audioPidArray[7] = {0,0,0,0,0,0,0};
uint16_t videoPidCounter, audioPidCounter = 0;

int main()
{

    parseConfigXML(CONFIG_FILE, &config);
    
    printf("aPid: %d, vPid: %d\n", config.aPid, config.vPid);
    
        
	struct timespec lockStatusWaitTime;
	struct timeval now;
    
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;

    

    if(Tuner_Init())
    {
        printf("\n%s : ERROR Tuner_Init() fail\n", __FUNCTION__);
        return -1;
    }

    if(Tuner_Register_Status_Callback(tunerStatusCallback))
    {
		printf("\n%s : ERROR Tuner_Register_Status_Callback() fail\n", __FUNCTION__);
	}

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

    pthread_mutex_lock(&statusMutex);
    if(ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("\n%s:ERROR Lock timeout exceeded!\n",__FUNCTION__);
        Tuner_Deinit();
        return -1;
    }
    pthread_mutex_unlock(&statusMutex);

    Player_Init(&playerHandle);
    Player_Source_Open(playerHandle, &sourceHandle);
    Demux_Register_Section_Filter_Callback(mySecFilterCallback);
    pmt_count++;
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
    printf("++EOParsing++\n");
    
    Player_Stream_Create(playerHandle,sourceHandle,VIDEO_PID,VIDEO_TYPE_MPEG2,&videoStreamHandle);
    Player_Stream_Create(playerHandle,sourceHandle,AUDIO_PID,AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
    
    pthread_create(&remote, NULL, &remoteThreadTask, NULL);
    while(main_running);

    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
    Player_Source_Close(playerHandle, sourceHandle);

jump:
    //pthread_join(remote, NULL);
    Player_Deinit(playerHandle);
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
    if(table_type==0x02) PMTparse(buffer);
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
    pmtTable.program_number = (uint16_t) ((*(buffer+3)<<8)  + *(buffer+4));
    pmtTable.program_info_lenght = (uint16_t) (((*(buffer+10)<<8) + *(buffer+11)) & 0x0FFF);
    pmtTable.stream_count = (uint8_t)(pmtTable.section_length - (12 + pmtTable.program_info_lenght + 4 - 3)) / 8;

    int i;
    uint8_t *m_buffer = (uint8_t *)(buffer) + 12 + pmtTable.program_info_lenght;
    
    for(i=0; i<pmtTable.stream_count;i++){

        stream.stream_type = m_buffer[0];
        stream.elementary_pid = (uint16_t) ((*(m_buffer+1)<<8) + *(m_buffer + 2)) & 0x1FFF;
        stream.ES_info_length = (uint16_t) ((*(m_buffer+3)<<8) + *(m_buffer + 4)) & 0x0FFF;
        m_buffer += 5 + stream.ES_info_length;

        pmtTable.streamArray[i] = stream;
    }
    
    pmtTableArray[pmtTableCount] = pmtTable;
    pmtTableCount++;
    

    printf("*********************************************\n");
    //printf("PMT parsed\n");
    return 0;
}

uint16_t getVideoElementaryPID(struct PmtTable pmtTable){
    int i;
    for(i=0;i<pmtTable.stream_count;i++)
    {
        if(pmtTable.streamArray[i].stream_type==2)
        {   //printf("Video pid: %d \n", pmtTable.streamArray[i].elementary_pid);
            return pmtTable.streamArray[i].elementary_pid; }
    }

    //printf("Error: VideoPID");
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

int32_t remoteMainFunction(){
    pthread_create(&remote, NULL, &remoteThreadTask, NULL);
    
    pthread_join(remote, NULL);
}

int32_t getKeys(int32_t count, uint8_t* buf, int32_t* eventsRead)
{
    int32_t ret = 0;
    
    /* read input events and put them in buffer */
    ret = read(inputFileDesc, buf, (size_t)(count * (int)sizeof(struct input_event)));
    if(ret <= 0)
    {
        printf("Error code %d", ret);
        return ERROR;
    }
    /* calculate number of read events */
    *eventsRead = ret / (int)sizeof(struct input_event);
    
    return NO_ERROR;
}

static void *remoteThreadTask() {
    

    uint32_t i;
    const char* dev = "/dev/input/event0";
    char deviceName[20];
    struct input_event* eventBuf;
    uint32_t eventCnt;
    inputFileDesc = open(dev, O_RDWR);
    
    if(inputFileDesc == -1)
    {
        printf("Error while opening device (%s) !", strerror(errno));
	    //return ERROR;
    }

    eventBuf = malloc(NUM_EVENTS * sizeof(struct input_event));
    if(!eventBuf)
    {
        printf("Error allocating memory !");
        //return ERROR;
    }

    ioctl(inputFileDesc, EVIOCGNAME(sizeof(deviceName)), deviceName);
	printf("RC device opened succesfully [%s]\n", deviceName);
    
    int running = 1;
    while(running)
    {
        /* read input eventS */
        if(getKeys(NUM_EVENTS, (uint8_t*)eventBuf, &eventCnt))
        {
			printf("Error while reading input events !");
			//return ERROR;
		}
 
        for(i = 0; i < eventCnt; i++)
        {
            /*
            printf("Event time: %d sec, %d usec\n",(int)eventBuf[i].time.tv_sec,(int)eventBuf[i].time.tv_usec);
            printf("Event type: %hu\n",eventBuf[i].type);
            printf("Event code: %hu\n",eventBuf[i].code);
            printf("Event value: %d\n",eventBuf[i].value);
            printf("\n");
            */
    
        if (eventBuf[i].type == 1 && (eventBuf[i].value == 1 || eventBuf[i].value ==2)) {
            switch(eventBuf[i].code) {
                

                case 2:{
                    channel= 1;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);   
                    //drawPressedButton()
                    break;
                }
                 
                case 3:{
                    channel = 2;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);   
                    //drawPressedButton();
                    break;
                }

                case 4:{
                    channel = 3;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    break;
                }
                
                case 5:{
                    channel = 4;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    break;
                }

                case 6:{
                    channel = 5;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    break;
                }
                
                case 7:{
                    channel = 6;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    break;
                }

                case 8:{
                    channel = 7;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    break;
                }
                
                case 9:{
                    channel = 8;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    break;
                }

                case 10:{
                    channel = 9;
                    printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    break;
                }

                
                case 61: {
                    if(channel>1){channel--;printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    } 
                    break;
                    
                }
                case 62: {
                    if(channel<9){channel++;printf("Kanal: %d\n", channel);
                    Player_Stream_Remove(playerHandle,sourceHandle,videoStreamHandle);
                    Player_Stream_Remove(playerHandle,sourceHandle,audioStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,videoPidArray[channel-1],VIDEO_TYPE_MPEG2,&videoStreamHandle);
                    Player_Stream_Create(playerHandle,sourceHandle,audioPidArray[channel-1],AUDIO_TYPE_MPEG_AUDIO,&audioStreamHandle);
                    //drawPressedButton();
                    }
                    break;
                }

                case 64: {
                    if(volume>1){volume--;printf("Volume minus \n");
                    //drawPressedButton();
                    } 
                    break;
                    
                }
                case 63: {
                    if(volume<100){volume++;printf("Volume plus \n");
                    //drawPressedButton();
                    }
                    break;
                }

                case 102:{
                    printf("Exit\n");
                    running=0;
                    main_running=0;
                    break;
                }
            }
        }
        }
    }
    
    free(eventBuf);
    return;
    //return NO_ERROR;
}

bool valueinarray(uint16_t val, uint16_t arr[], size_t n) {
    int i;
    for(i = 0; i < n; i++) {
        if(arr[i] == val)
            return true;
    }
    return false;
}

int32_t PMTparse(uint8_t *buffer){
    pmt[pmt_count].section_length = (uint16_t) (((*(buffer+1) << 8) + *(buffer + 2)) & 0x0FFF);
    pmt[pmt_count].program_number = (uint16_t) (((*(buffer+3) << 8) + *(buffer + 4)));
    pmt[pmt_count].program_info_lenght = (uint16_t) (((*(buffer+10) << 8) + *(buffer + 11)) & 0x0FFF);
   // int stream_count=(uint8_t)(pmt[i].section_length - (12 + pmt[i].program_info_lenght + 4 - 3))/8;
    //printf("%d,%d,%d,%d\n",pmt[i].section_length,pmt[i].program_number,pmt[i].program_info_lenght, stream_count);
    //section_len-(12+program_info_len+4-3)/8
    uint8_t *m_buffer = (uint8_t*)(buffer) + 12 + pmt[pmt_count].program_info_lenght;
    int j;
    for (j= 0; j<10; j++){
            pmt[pmt_count].stream_type[j] = m_buffer[0];
            pmt[pmt_count].elemetary_pid[j] = (uint16_t) (((*(m_buffer+1)<<8) + *(m_buffer +2)) & 0x1FFF);
            pmt[pmt_count].ES_info_lenght[j] = (uint16_t) (((*(m_buffer+3)<<8) + *(m_buffer +4)) & 0x0FFF);
            printf("program num:%d,elem_pid: %d,ES_info: %d\n",pmt[pmt_count].program_number,pmt[pmt_count].elemetary_pid[j],pmt[pmt_count].ES_info_lenght[j]);
            m_buffer += 5 + pmt[pmt_count].ES_info_lenght[j];
        
            if(pmt[pmt_count].stream_type[j] == 2){
                if(!valueinarray(pmt[pmt_count].elemetary_pid[j],videoPidArray,7))
                {
                    videoPidArray[videoPidCounter] = pmt[pmt_count].elemetary_pid[j];
                    printf("Added videoPid: %d \n ", videoPidArray[videoPidCounter]);
                    videoPidCounter++;
                }
            }

            if(pmt[pmt_count].stream_type[j] == 3){
                if(!valueinarray(pmt[pmt_count].elemetary_pid[j],audioPidArray,7))
                {
                    audioPidArray[audioPidCounter] = pmt[pmt_count].elemetary_pid[j];
                    printf("Added audioPid: %d \n ", audioPidArray[audioPidCounter]);
                    audioPidCounter++;
                }
            }
        }


}

/* int32_t drawPressedButton(){ 
    char channel_str[2];
    sprintf( channel_str, "%d", channel );
    char volume_str[3];
    sprintf( volume_str, "%d", volume );

    DFBCHECK(primary->SetColor(primary, 0x00,0x00,0x00,0xff));
	DFBCHECK(primary->FillRectangle(primary,0,0,screenWidth,screenHeight));
   
    DFBCHECK(primary->SetColor(primary, 0xff,0x00,0x00,0xff));
    DFBCHECK(primary->DrawString(primary,channel_str,1,screenWidth/10,screenHeight/10, DSTF_CENTER));
    DFBCHECK(primary->DrawString(primary,volume_str,3,screenWidth/2,screenHeight/10, DSTF_CENTER));

    DFBCHECK(primary->Flip(primary,NULL,0));
    timer_reset();
    // timer_reset2();
    return 0;
} */
