#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "elog_api.h"

int DoELog(const char *host,
           const char *sysname,
           const char *sender,
           const char *title,
           const char *message,
           const char *msgcharset)
{
  struct sockaddr_in   serv_addr;
  struct hostent      *hp;
  int                  sock;
  int                  r;
  char                 buf [10000];
  char                 body[10000];
  char                 junk[100];

    /* Be sure we have a server name */
    if (host == NULL  ||  *host == '\0') host = getenv(ELOG_HOST_ENV);
    if (host == NULL  ||  *host == '\0')
    {
        errno = 0;
        return -1;
    }

    if (title   == NULL) title   = "UNTITLED";
    if (message == NULL) message = "";
    
#if 0
    fprintf(stderr, "Logging to '%s', sys='%s', sender='%s', msg=\"%s\"\n",
            host, sysname, sender, message);
    return 0;
#endif

    /* Obtain server IP */
    if ((hp = gethostbyname(host)) == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    /* Prepare address... */
    bzero (&serv_addr, sizeof(serv_addr));
    memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
    serv_addr.sin_family = hp->h_addrtype;
    serv_addr.sin_port   = htons(80);

    /* ...create a socket... */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* ...and establish a connection */
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        close(sock);
        return -1;
    }

    {
      time_t     timenow;
      struct tm *ltime = 0;
      char data_filename  [80];
      char strtim_for_post[80];
      char strtim_for_time[80];
      char strtim_for_date[80];

      static const char* boundary = "1a2b3c4d5e6f7g8h9i0";     // for possible sequences see RFC2046
      
      const char *severity = "NONE";
      const char *location = "not set";
      const char *keywords = "not set";
      const char *author   = (sender != NULL  && *sender != '\0')? sender : "UNKNOWN";
      const char *category = "USERLOG";
      const char *backlink = "http://dragon.inp.nsk.su/ebookelog"; // ???
      const char *image_filename = "";
      const char *experts = "";
      const char *email = "";
      const char *topic = "subject1";
      
        // Get current time in appropriate formats (e.g. 2007-03-28T09:20:15-00):
        timenow = time(0);
        ltime = localtime(&timenow);
        
        strftime(data_filename,   sizeof (data_filename),   "%Y-%m-%dT%H:%M:%S-00.xml", ltime);
        strftime(strtim_for_post, sizeof (strtim_for_post), "%Y/%m",                    ltime);
        strftime(strtim_for_time, sizeof (strtim_for_time), "%H:%M:%S-00",              ltime);
        strftime(strtim_for_date, sizeof (strtim_for_date), "%d.%m.%Y",                 ltime);

        // Fill request body:
        
        sprintf (body, "--%s\r\n"
                 "Content-Disposition: form-data; name=\"severity\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"location\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"keywords\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"time\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"date\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"author\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"title\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"text\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"category\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"metainfo\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"backlink\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"image\"; filename=\"%s\"\r\n"
                 "Content-Type: application/octet-stream\r\n\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"experts\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"email\"\r\n\r\n"
                 "%s\r\n"
                 "--%s\r\n"
                 "Content-Disposition: form-data; name=\"topic\"\r\n\r\n"
                 "%s\r\n"
                 "--%s--\r\n", boundary, severity, boundary, location, boundary, keywords, boundary,
                 strtim_for_time, boundary, strtim_for_date, boundary, author, boundary,
                 title, boundary, message, boundary, category, boundary, data_filename,
                 boundary, backlink, boundary, image_filename, boundary, experts,
                 boundary, email, boundary, topic, boundary);
        
        sprintf (buf, "POST /elog/servlet/FileEdit?source=/ebookelog/data/%s/%s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\n"
                 "Accept-Charset: KOI8-R,utf-8;q=0.7,*;q=0.7\r\n"
                 "Keep-Alive: 300\r\n"
                 "Connection: keep-alive\r\n"
                 "Content-Type: multipart/form-data; boundary=%s\r\n"
                 "Content-Length: %zu\r\n\r\n%s", strtim_for_post, data_filename, host, boundary, strlen(body), body);
    }
    
    /* Send */
    if (write(sock, buf, strlen(buf)) < 0)
    {
        close(sock);
        return -1;
    }

    /* Get and dump the response */
    while ((r = read(sock, &junk, sizeof(junk))) != 0)
    {
        if (r < 0)
        {
            close(sock);
            return -1;
        }
    }

    /* We have finished */
    close(sock);
    
    return 0;
}

int IsELogPossible(const char *host)
{
    if (host != NULL  &&  *host != '\0') return 1;
    host = getenv(ELOG_HOST_ENV);
    if (host != NULL  &&  *host != '\0') return 1;
    return 0;
}
