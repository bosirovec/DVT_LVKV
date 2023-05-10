#include <stdio.h>
#include <directfb.h>
#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>


#define NUM_EVENTS  5
#define NON_STOP    1
#define NO_ERROR 		0
#define ERROR			1

int32_t remoteMainFunction();
int32_t getKey(int32_t count, uint8_t* buf, int32_t* eventsRead);
static void *remoteThreadTask();
int32_t drawPressedButton();
static pthread_t remote;
int main_running = 1;
static int32_t inputFileDesc;
int channel = 0;
int volume = 50;

/// STVARI ZA TIMER
timer_t timerId;
int32_t timerFlags = 0;
struct itimerspec timerSpec;
struct itimerspec timerSpecOld;
timer_t timerId2;
int32_t timerFlags2 = 0;
struct itimerspec timerSpec2;
struct itimerspec timerSpecOld2;

void volumeManager();
void drawVolume(int boxesNumber);

void timer_init();
void timer_deinit();
void timer_reset();
void clear_screen_notify();
void clear_screen();

void timer_init2();
void timer_deinit2();
void timer_reset2();
void clear_screen_notify2();
void clear_screen2();
///

/* helper macro for error checking */
#define DFBCHECK(x...)                                      \
{                                                           \
DFBResult err = x;                                          \
                                                            \
if (err != DFB_OK)                                          \
  {                                                         \
    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );  \
    DirectFBErrorFatal( #x, err );                          \
  }                                                         \
}


/// INTERFACE STVARI
static IDirectFBSurface *primary = NULL;
IDirectFB *dfbInterface = NULL;
static int screenWidth = 0;
static int screenHeight = 0;
DFBSurfaceDescription surfaceDesc;
///

IDirectFBImageProvider *provider;
IDirectFBSurface *logoSurface = NULL;
int32_t logoHeight, logoWidth;



int32_t main(int32_t argc, char** argv)
{
    timer_init();
    //    timer_init2();

    /* initialize DirectFB */
    
	DFBCHECK(DirectFBInit(&argc, &argv));
    /* fetch the DirectFB interface */
	DFBCHECK(DirectFBCreate(&dfbInterface));
    /* tell the DirectFB to take the full screen for this application */
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
	
    
    /* create primary surface with double buffering enabled */
    
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));
    
    
    /* fetch the screen size */
    DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));
    
    
    /* clear the screen before drawing anything (draw black full screen rectangle)*/
    
    DFBCHECK(primary->SetColor(/*surface to draw on*/ primary,
                               /*red*/ 0x00,
                               /*green*/ 0x00,
                               /*blue*/ 0x00,
                               /*alpha*/ 0xff));
	DFBCHECK(primary->FillRectangle(/*surface to draw on*/ primary,
                                    /*upper left x coordinate*/ 0,
                                    /*upper left y coordinate*/ 0,
                                    /*rectangle width*/ screenWidth,
                                    /*rectangle height*/ screenHeight));
	
    
    /* rectangle drawing */
    
    DFBCHECK(primary->SetColor(primary, 0x03, 0x03, 0xff, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/5, screenHeight/5, screenWidth/3, screenHeight/3));
    
    
	/* line drawing */
    
	DFBCHECK(primary->SetColor(primary, 0xff, 0x80, 0x80, 0xff));
	DFBCHECK(primary->DrawLine(primary,
                               /*x coordinate of the starting point*/ 150,
                               /*y coordinate of the starting point*/ screenHeight/3,
                               /*x coordinate of the ending point*/screenWidth-200,
                               /*y coordinate of the ending point*/ (screenHeight/3)*2));
	
    
	/* draw text */

	IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	
    /* specify the height of the font by raising the appropriate flag and setting the height value */
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = 48;
	
    /* create the font and set the created font for primary surface text drawing */
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));
    
    /* draw the text */
	DFBCHECK(primary->DrawString(primary,
                                 /*text to be drawn*/ "Text Example",
                                 /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
                                 /*x coordinate of the lower left corner of the resulting text*/ 100,
                                 /*y coordinate of the lower left corner of the resulting text*/ 100,
                                 /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
	
    
	/* draw image from file */
	
    /* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "dfblogo_alpha.png", &provider));
    /* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
    /* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
    /* render the image to the surface */
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));
	    /* cleanup the provider after rendering the image to the surface */
	provider->Release(provider);
	
    /* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	DFBCHECK(primary->Blit(primary,
                           /*source surface*/ logoSurface,
                           /*source region, NULL to blit the whole surface*/ NULL,
                           /*destination x coordinate of the upper left corner of the image*/50,
                           /*destination y coordinate of the upper left corner of the image*/screenHeight - logoHeight -50));
    
    
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary,
                       /*region to be updated, NULL for the whole surface*/NULL,
                       /*flip flags*/0));
    
    
    /* wait 5 seconds before terminating*/
	sleep(1);

    remoteMainFunction();
    
    /*clean up*/
    
	primary->Release(primary);
	dfbInterface->Release(dfbInterface);
    
    //  timer_deinit2();
    timer_deinit();


    return 0;
}

int32_t remoteMainFunction(){
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
                    drawPressedButton();
                    break;
                }
                 
                case 3:{
                    channel = 2;
                    printf("Kanal: %d\n", channel);
                    drawPressedButton();
                    break;
                }

                case 4:{
                    channel = 3;
                    printf("Kanal: %d\n", channel);
                    drawPressedButton();
                    break;
                }
                
                case 5:{
                    channel = 4;
                    printf("Kanal: %d\n", channel);
                    drawPressedButton();
                    break;
                }

                case 6:{
                    channel = 5;
                    printf("Kanal: %d\n", channel);
                    drawPressedButton();
                    break;
                }
                
                case 7:{
                    channel = 6;
                    printf("Kanal: %d\n", channel);
                    drawPressedButton();
                    break;
                }

                case 8:{
                    channel = 7;
                    printf("Kanal: %d\n", channel);
                    drawPressedButton();
                    break;
                }
                
                case 9:{
                    channel = 8;
                    printf("Kanal: %d\n", channel);
                    drawPressedButton();
                    break;
                }

                case 10:{
                    channel = 9;
                    printf("Kanal: %d\n", channel);
                    drawPressedButton();
                    break;
                }

                
                case 61: {
                    if(channel>1){channel--;printf("Kanal: %d\n", channel);drawPressedButton();} 
                    break;
                    
                }
                case 62: {
                    if(channel<9){channel++;printf("Kanal: %d\n", channel);drawPressedButton();}
                    break;
                }

                case 64: {
                    if(volume>1){volume--;printf("Volume minus \n");drawPressedButton();} 
                    break;
                    
                }
                case 63: {
                    if(volume<100){volume++;printf("Volume plus \n");drawPressedButton();}
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

int32_t drawPressedButton(){
    char channel_str[2];
    sprintf( channel_str, "%d", channel );
    
    
    char *ch = "Ch. ";

    DFBCHECK(primary->SetColor(primary, 0x00,0x00,0x00,0xff));
	DFBCHECK(primary->FillRectangle(primary,0,0,screenWidth,screenHeight));
   
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "krug.png", &provider));
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));
	provider->Release(provider);
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	DFBCHECK(primary->Blit(primary, logoSurface,
                           /*source region, NULL to blit the whole surface*/ NULL,
                           /*destination x coordinate of the upper left corner of the image*/125,
                           /*destination y coordinate of the upper left corner of the image*/25));

    DFBCHECK(primary->SetColor(primary, 0x00,0x00,0xff,0xff));
    DFBCHECK(primary->DrawString(primary,ch,4,screenWidth/10,screenHeight/10, DSTF_CENTER));
    DFBCHECK(primary->DrawString(primary,channel_str,1,screenWidth/10 + 50,screenHeight/10, DSTF_CENTER));
    
    

    volumeManager();


    DFBCHECK(primary->Flip(primary,NULL,0));
    timer_reset();
    // timer_reset2();
    return 0;
}

void timer_init(){
    struct sigevent signalEvent;
    signalEvent.sigev_notify = SIGEV_THREAD;
    signalEvent.sigev_notify_function = clear_screen_notify;
    signalEvent.sigev_value.sival_ptr = NULL;
    signalEvent.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME,&signalEvent,&timerId);
    
    memset(&timerSpec,0,sizeof(timerSpec));
    timerSpec.it_value.tv_sec = 5;
    timerSpec.it_value.tv_nsec = 0;

}

void timer_deinit(){
    memset(&timerSpec,0,sizeof(timerSpec));
    timer_settime(timerId, timerFlags,&timerSpec,&timerSpecOld);
    timer_delete(timerId);

}

void timer_reset(){
    timer_settime(timerId, timerFlags,&timerSpec,&timerSpecOld);
}

void clear_screen(){
    primary->SetColor(primary, 0x00,0x00,0x00,0xff);
	primary->FillRectangle(primary,0,0,screenWidth/2,screenHeight);
}

void clear_screen_notify(){
    clear_screen();
    primary->Flip(primary,NULL,0);
}

void volumeManager(){

    //x_start i y_start oznacavaju gornji lijevi rub najvišeg volume boxa
    int x_start = screenWidth/10 * 9;
    int y_start  = 200;

    //Crta sivu pozadinu
    DFBCHECK(primary->SetColor(primary, 0x80,0x80,0x80,0xf0));
    DFBCHECK(primary->FillRectangle(primary,x_start-50,y_start-50,screenWidth/10-50,500));
    
    //Crni ispis vrijednosti volumea
    char volume_str[3];
    sprintf( volume_str, "%d", volume );
    DFBCHECK(primary->SetColor(primary, 0x00,0x00,0x00,0xff));
    if(volume==100){
        DFBCHECK(primary->DrawString(primary,volume_str,3,x_start + 20,y_start+430, DSTF_CENTER));
    }
    else if(volume<10){
        DFBCHECK(primary->DrawString(primary,volume_str,1,x_start + 20,y_start+430, DSTF_CENTER));
    }
    else{
        DFBCHECK(primary->DrawString(primary,volume_str,2,x_start + 20,y_start+430, DSTF_CENTER));
    }
    
    if(volume==100){
        drawVolume(10);
    }
    if(volume<100&&volume>90){
        drawVolume(9);
    }
    if(volume<=90&&volume>80){
        drawVolume(8);
    }
    if(volume<=80&&volume>70){
        drawVolume(7);
    }
    if(volume<=70&&volume>60){
        drawVolume(6);
    }
    if(volume<=60&&volume>50){
        drawVolume(5);
    }
    if(volume<=50&&volume>40){
        drawVolume(4);
    }
    if(volume<=40&&volume>30){
        drawVolume(3);
    }
    if(volume<=30&&volume>20){
        drawVolume(2);
    }
    if(volume<=20&&volume>10){
        drawVolume(1);
    }
    if(volume<=10){
        drawVolume(11);
        
    }
    

}

void drawVolume(int boxesNumber){
    int x_start = screenWidth/10 * 9;
    int y_start  = 200;
    int difference = 40;
    int i = 0;

    //nacrtaj 10-x blijedih i x punih - Ako je 11, crta 10 praznih
    if(boxesNumber==11){
            for(i=0;i<10;i++){
            DFBCHECK(primary->SetColor(primary, 0x00,0x00,0xff,0x08));
            DFBCHECK(primary->FillRectangle(primary,x_start,y_start+i*difference,50,20));
        }
    }
    else
    {
        DFBCHECK(primary->SetColor(primary, 0x00,0x00,0xff,0x08));
        for(i = 0; i<10-boxesNumber; i++){
            DFBCHECK(primary->FillRectangle(primary,x_start,y_start+i*difference,50,20));
        }
        DFBCHECK(primary->SetColor(primary, 0x00,0x00,0xff,0xff));
        for(i = 10-boxesNumber; i<10; i++ )
        {
            DFBCHECK(primary->FillRectangle(primary,x_start,y_start+i*difference,50,20));
        }
    }
    

}