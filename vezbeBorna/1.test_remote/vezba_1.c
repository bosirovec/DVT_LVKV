#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>


#define NUM_EVENTS  5

#define NON_STOP    1

/* error codes */
#define NO_ERROR 		0
#define ERROR			1

static int32_t inputFileDesc;

int32_t getKeys(int32_t count, uint8_t* buf, int32_t* eventRead);

static pthread_t remote;

int main_running = 1;

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
    
    int running = 1,channel = 0;
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
                    channel = 1;
                    printf("Kanal: %d\n", channel);
                    break;
                }
                 
                case 3:{
                    channel = 2;
                    printf("Kanal: %d\n", channel);
                    break;
                }

                case 4:{
                    channel = 3;
                    printf("Kanal: %d\n", channel);
                    break;
                }
                
                case 5:{
                    channel = 4;
                    printf("Kanal: %d\n", channel);
                    break;
                }

                case 6:{
                    channel = 5;
                    printf("Kanal: %d\n", channel);
                    break;
                }
                
                case 7:{
                    channel = 6;
                    printf("Kanal: %d\n", channel);
                    break;
                }

                case 8:{
                    channel = 7;
                    printf("Kanal: %d\n", channel);
                    break;
                }
                
                case 9:{
                    channel = 8;
                    printf("Kanal: %d\n", channel);
                    break;
                }

                case 10:{
                    channel = 9;
                    printf("Kanal: %d\n", channel);
                    break;
                }

                
                case 61: {
                    if(channel>1){channel--;printf("Kanal: %d\n", channel);} 
                    break;
                    
                }
                case 62: {
                    if(channel<9){channel++;printf("Kanal: %d\n", channel);}
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

int32_t main()
{
    pthread_create(&remote, NULL, &remoteThreadTask, NULL);
    while(main_running);
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
