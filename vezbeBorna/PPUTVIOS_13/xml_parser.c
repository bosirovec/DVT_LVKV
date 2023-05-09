#include "xml_parser.h"

void parseConfigXML(char *filename, struct Config *config){

    int has_frequency,has_bandwith,has_apid,has_vpid,has_atype,has_vtype,has_channel_index,has_satype,has_svtype,has_time,has_module=0;

    char buf[800];
    FILE *file;
    
    file = fopen(filename,"r");
    
    memset(buf,0,800);
    while(fgets(buf,sizeof buf, file))
    {
        if(buf[strspn(buf," ")]=='\n')
            continue;
        
        
        if(sscanf(buf,"<config>\n") == 1){}
        else
        if(sscanf(buf,"\t<frequency>%d</frequency>\n", &config->frequency) == 1)
            {has_frequency = 1;}
        else
        if(sscanf(buf,"\t<bandwidth>%d</bandwidth>", &config->bandwidth) == 1)
            {has_bandwith = 1;}
        else
        if(sscanf(buf,"\t<module>%s</module>", &config->module) == 1)
            {has_module = 1;}
        else
        if(sscanf(buf,"\t\t<apid>%d</apid>", &config->aPid) == 1)
            {has_apid = 1;}
        else
        if(sscanf(buf,"\t\t<vpid>%d</vpid>", &config->vPid) == 1)
            {has_vpid = 1;}
        else
        if(sscanf(buf,"\t\t<atype>%s</atype>\n", &config->satype) == 1)
            {   
                char tmp_buff[4];
                strncpy(tmp_buff,buf+9,3);
                tmp_buff[3] = '\0';
                if(!strcmp(tmp_buff,(char*)"ac3")){config->aType=10;}
                has_atype,has_satype = 1; 
            }
        else
        if(sscanf(buf,"\t\t<vtype>%s</vtype>", &config->svtype) == 1)
            {
                char tmp_buff[6];
                strncpy(tmp_buff,buf+9,5);
                tmp_buff[5] = '\0';
                if(!strcmp(tmp_buff,(char*)"mpeg2")){config->vType=42;}
                has_vtype,has_svtype = 1; 
            }
        else
        if(sscanf(buf,"\t\t<time>%s</time>", &config->time) == 1)
            {has_time = 1;}        
        else
        if(sscanf(buf,"\t\t<channel_index>%d</channel_index>", &config->channel_index) == 1)
            {has_channel_index = 1;}
        //else
          //  fprintf(stderr, "invalid line: %s", buf);
        
        memset(buf,0,800);
    }
    
    //printf("\n");
    fclose(file);
    
    return;    
}
