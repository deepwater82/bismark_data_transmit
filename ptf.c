#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>

#include <curl/curl.h>
#include <dirent.h>
#include <sys/inotify.h>

#define DIR_PATH "/home/hyojoon/Work/netgear/bismark/ptf/test_dir/"
#define URL_PATH "http://bourbon.gtnoise.net/"
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

    
typedef char * string;

int curl_send(CURL* curl, char* filename)
{
    char fbuffer[128];
    char ubuffer[128];
    bzero(fbuffer,128);
    bzero(ubuffer,128);
    strcpy(fbuffer,DIR_PATH);
    strcpy(ubuffer,URL_PATH);
    
    char* file = strcat(fbuffer, filename);
    char* ufile = strcat(ubuffer, filename);
    FILE* hd_src;
    int hd;
    struct stat file_info;
    CURLcode res;

    char errbuf[CURL_ERROR_SIZE];
    bzero(errbuf,CURL_ERROR_SIZE);
    
    /* get the file size of the local file */ 
    hd = open(file, O_RDONLY);
    fstat(hd, &file_info);
    close(hd);

    /* get a FILE * of the same file, could also be made with
     *      fdopen() from the previous descriptor, but hey this is just
     *           an example! */ 
    hd_src = fopen(file, "rb");

    if(curl)
    {
        /* enable error messages */ 
        curl_easy_setopt(curl,CURLOPT_ERRORBUFFER, errbuf);

        /* specify target URL, and note that this URL should include a file
         *        name, not only a directory */ 
        curl_easy_setopt(curl, CURLOPT_URL, ufile);

        /* now specify which file to upload */ 
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

        /* provide the size of the upload, we specicially typecast the value
         *        to curl_off_t since we must be sure to use the correct data size */ 
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

        /* Now run off and do what you've been told! */ 
        res = curl_easy_perform(curl);
    }

    if(res)
    {
        printf("Failed perform.\n");
        printf("ERR:%s\n", errbuf);
    }
    else
        printf("WORKED!\n");

    fclose(hd_src);
    return 0;
}

int main(int argc, char** argv)
{
    int ifd;
    int wd;
    char buffer[BUF_LEN];
    int length = 0;
    int i = 0;

    CURL *curl;

    bzero(buffer,BUF_LEN);

    // initialize cURL, setup options
    curl = curl_easy_init();
    if(curl) {
        
        /* enable uploading */ 
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* HTTP PUT please */ 
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    }

    // Initialize inotify
    if( (ifd=inotify_init())<0)
    {
        perror( "inotify_init" );
        return -1;
    }

    // add watch 
    wd = inotify_add_watch( ifd, DIR_PATH, IN_MODIFY | IN_CREATE | IN_DELETE );

    while(1)
    {
        i = 0;
        bzero(buffer,BUF_LEN);
        length = read(ifd, buffer, BUF_LEN);  

        if ( length < 0 )
            perror( "read" );

        while ( i < length ) 
        {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
            if ( event->len ) 
            {
                printf("Event happened.\n");
                if (strstr(event->name, "swp"))
                {
                    printf("swp file. skip\n");
                }
                else
                {
                    if (event->mask & IN_MODIFY)
                    {
                        printf("Modify event. File=%s\n", event->name);
                        curl_send(curl, event->name);
                    }
                    if (event->mask & IN_CREATE)
                    {
                        printf("Create event. File=%s\n", event->name);
                    }
                    if (event->mask & IN_DELETE)
                    {
                        printf("Delete event. File=%s\n", event->name);
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        }
    }

    /* always cleanup */ 
    curl_easy_cleanup(curl);

    return 0;
}
