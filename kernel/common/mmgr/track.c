#include "common/sedna.h"
#include "common/errdbg/d_printf.h"
#include "stdio.h"

#ifdef SE_MEMORY_TRACK

#undef malloc
#undef free
#undef realloc

#endif /* SE_MEMORY_TRACK */

/// Comment next string to get unfreed dump into screen.
#define SE_MEMORY_DUMP_TO_FILE

#define SE_MEMORY_TRACK_FILENAME_BU_BASE  "MT-"
#define SE_MEMORY_TRACK_FILENAME_BU_SUFX  ".xml"

struct ALLOC_INFO;
struct ALLOC_INFO_LIST;

typedef struct ALLOC_INFO {
	void* address;
	struct ALLOC_INFO* next;
    usize_t size;
    char file[64];
    int line;
} ALLOC_INFO;

typedef struct ALLOC_INFO_LIST
{
    ALLOC_INFO* top;
    void  (*insert) (struct ALLOC_INFO_LIST* list, ALLOC_INFO* info);
    void  (*remove) (struct ALLOC_INFO_LIST* list, void* ptr);

} ALLOC_INFO_LIST;

ALLOC_INFO_LIST *allocList;

void insert_alloc_list(ALLOC_INFO_LIST* list, ALLOC_INFO* info)
{
    info->next = list->top;
    list->top = info;
}

void remove_alloc_list(ALLOC_INFO_LIST* list, void* ptr)
{
    ALLOC_INFO*  itr             = list->top;
    ALLOC_INFO** ptr_to_next_ptr = &(list->top);
    while(itr)
    {
        if(itr->address == ptr)
        {
            ALLOC_INFO* next_cur = itr->next;
            free(itr);
            *ptr_to_next_ptr = next_cur; 
            break;
        }
        else
        {
            ptr_to_next_ptr = &(itr->next);
            itr = itr->next;
        }
    }
}

void init_alloc_list(ALLOC_INFO_LIST* list)
{
    list->top = NULL;
    list->insert = &insert_alloc_list;
    list->remove = &remove_alloc_list;
}

void AddTrack(void* addr, usize_t asize, const char *fname, int lnum)
{
    ALLOC_INFO *info;

    if(!allocList) {
	    allocList = (ALLOC_INFO_LIST*)malloc(sizeof(ALLOC_INFO_LIST));
	    init_alloc_list(allocList);
	}

    info = (ALLOC_INFO*)malloc(sizeof(ALLOC_INFO));
    info->next = NULL;
    info->address = addr;
    strncpy(info->file, fname, 63);
    info->line = lnum;
    info->size = asize;
    allocList->insert(allocList, info);
}

void RemoveTrack(void* addr)
{
    if(!allocList) return;
    allocList->remove(allocList, addr);
}

const char* __get_component_string_name(int component)
{
    const char* component_c_str = NULL;
    switch(component)
    {
        switch (component)
        {
            case EL_CDB:  component_c_str = "CDB";  break; 
            case EL_DDB:  component_c_str = "DDB";  break; 
            case EL_GOV:  component_c_str = "GOV";  break; 
            case EL_RC:   component_c_str = "RC";   break; 
            case EL_SM:   component_c_str = "SM";   break; 
            case EL_SMSD: component_c_str = "SMSD"; break;
            case EL_STOP: component_c_str = "STOP"; break;
            case EL_TRN:  component_c_str = "TRN";  break;
            default:      component_c_str = "UNK";
        }
    }
    return component_c_str;
}


void DumpUnfreed(int component) 
{
    int totalSize = 0;
    ALLOC_INFO* itr = NULL;
#ifdef SE_MEMORY_DUMP_TO_FILE
    FILE *du_ostr = NULL;
    char buf[SEDNA_DATA_VAR_SIZE + 128];
    char dt_buf[32];
    struct tm *newtime;
    time_t aclock;
#endif

    if(!allocList) return;
    itr = allocList->top;

#ifdef SE_MEMORY_DUMP_TO_FILE
    time(&aclock);                   /* Get time in seconds */
    newtime = localtime(&aclock);    /* Convert time to struct tm form */

    sprintf(dt_buf,"%04d-%02d-%02d-%02d-%02d-%02d",
            newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday,
            newtime->tm_hour, newtime->tm_min, newtime->tm_sec);

    strcpy(buf, SEDNA_DATA);
#ifdef _WIN32
    strcat(buf, "\\data\\");
#else
    strcat(buf, "/data/");
#endif
    strcat(buf, SE_MEMORY_TRACK_FILENAME_BU_BASE);
    strcat(buf,  __get_component_string_name(component));
    strcat(buf, "-");
    strcat(buf, dt_buf);
    strcat(buf, SE_MEMORY_TRACK_FILENAME_BU_SUFX);

    du_ostr = fopen(buf, "w");

    if (!du_ostr) 
    {
         d_printf1("Memory Tracking Error: Could not create file to dump unfreed!\n");
         return;
    }

    fprintf(du_ostr, "<?xml version=\"1.0\" standalone=\"yes\"?>\n");
    fprintf(du_ostr, "<unfreed>\n");
    fprintf(du_ostr, "\t<component>%s</component>\n", __get_component_string_name(component));
#endif 

    while(itr) {

#ifndef SE_MEMORY_DUMP_TO_FILE
        printf("%-50s:\t\tLINE %d,\t\tADDRESS %d\t%d unfreed\n", itr->file, itr->line, itr->address, itr->size);
#else
        fprintf(du_ostr, "\t<block>\n");
        fprintf(du_ostr, "\t\t<file>%s</file>\n",       itr->file);
        fprintf(du_ostr, "\t\t<line>%d</line>\n",       itr->line);
        fprintf(du_ostr, "\t\t<address>%d</address>\n", itr->address);
        fprintf(du_ostr, "\t\t<size>%d</size>\n",       itr->size);
        fprintf(du_ostr, "\t</block>\n");

#endif

        totalSize += itr->size;
        itr = itr->next;
    }

#ifndef SE_MEMORY_DUMP_TO_FILE        	
	printf("-----------------------------------------------------------\n");
    printf("Total Unfreed: %d bytes\n", totalSize);
#else
    fprintf(du_ostr, "     <total>%d</total>\n</unfreed>", totalSize);

    fflush(du_ostr);
    fclose(du_ostr);
    du_ostr = NULL;
#endif

}