/**
   @file owbmonsvr.c
   @author Glen Wiley <glen.wiley@gmail.com>
   @brief owbmonsvr network server that accepts data form owbmonsvr nodes
   @details network server that accepts multiple concurrent clients
            intended to provide reporting interface to owbmonsvr nodes
            runs continuously, exits != 0 if cant create a socket to listen on

TODO: signal handler to remove run file
TODO: proper syslog style error logging
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

#define PORT       1969
#define MAXMSGLEN  512
#define MAXCFGLINE 255
#define MAXPIDLEN  30
// backlog for listen for server socket
#define LSTNBACKLOG 5

// max number of sensors on a node
#define NSENSORS 4

#define RUNFILE "/var/run/lock/owbmonsvr.pid"

// used to store calibration data for a specific node
// values in calibration fields are added to values received from node
struct nodecfg_st
{
   char *mac;
   char *name;
   // names of sensors, emtpy string = not connected
   char *sensors[NSENSORS];
   // calibration adjustment for each sensor
   int  cal[NSENSORS];
};

// this has to be global so that the atexit() function can work
char *fn_run;

//---------------------------------------- readnodecfg
/**
  read node configuration file into array of node configs
  @param fn file name to read configuration from
  @returns null terminated array of node configurations
  @details one spare delimited line per device, <MAC> <NAME> <CAL1> <CAL2> <CAL3> <CAL4>
*/
struct nodecfg_st **
readnodecfg(char *fn)
{
    FILE   *f;
    int    nnodes = 0;
    int    ntok;
    int    s;
    int    lineno = 0;
    char   txt[MAXCFGLINE+1];
    char   *tok;
    char   *p;
    struct nodecfg_st **nodecfgs = NULL;
    struct nodecfg_st *nodecfg = NULL;
    struct nodecfg_st *node;

    nodecfgs = (struct nodecfg_st **) malloc(sizeof(struct nodecfg_st *));
    nodecfgs[0] = NULL;

#ifdef DEBUG
    printf("DEBUG: readnodecfg()\n");
#endif

    f = fopen(fn, "r");
    if(f == NULL)
    {
        perror(" unable to open node config file");
        exit(1);
    }
    else
    {
        // TODO: report errors in config file syntax

        while(fgets(txt, MAXCFGLINE, f) != NULL)
        {
            lineno++;
            if(txt[0] != '#' && txt[0] != '\n')
            {
                // TODO: bail on malloc error
                node = (struct nodecfg_st *) malloc(sizeof(struct nodecfg_st));
                node->mac      = strdup("");
                node->name     = strdup("");
                for(s=0; s++; s<NSENSORS)
                {
                    node->sensors[s] = NULL;
                    node->cal[s]     = 0;
                }

                tok  = strtok(txt, " \n");
                ntok = 0;
                while(tok != NULL && *tok != '\0')
                {
                    if(ntok == 0)
                    {
                        free(node->mac);
                        node->mac = strdup(tok);
                    }
                    else if(ntok == 1)
                    {
                        free(node->name);
                        node->name = strdup(tok);
                    }
                    else if(ntok >= 2)
                    {
                        // field format is "<name>:<cal>"
                        // no name means no sensor is connected
                        if(tok[0] != ':')
                        {
                            p = strchr(tok, ':');
                            if(*p != '\0')
                            {
                                *p = '\0';
                                node->sensors[ntok-2] = strdup(tok);
                                p++;
                                if(*p != '\0')
                                    node->cal[ntok-2] = atoi(p);
                                else
                                    node->cal[ntok-2] = 0;
                            }
                        }
                    }

                    ntok++;
                    tok = strtok(NULL, " \n");
                } // while tok

                nnodes++;
                nodecfgs = (struct nodecfg_st **) realloc(nodecfgs
                 , nnodes * sizeof(struct nodecfg_st *));
                nodecfgs[nnodes-1] = node;
            } // if txt
        } // while
        fclose(f);
    } // if f else

    nodecfgs = (struct nodecfg_st **) realloc(nodecfgs
     , (nnodes+1) * sizeof(struct nodecfg_st *));
    nodecfgs[nnodes] = NULL;

#ifdef DEBUG
    nnodes = 0;
    while(nodecfgs && (nodecfg = nodecfgs[nnodes]) != NULL)
    {
        printf("DEBUG: node=%d, mac=%s, name=%s"
         , nnodes, nodecfg->mac, nodecfg->name);
        for(s=0; s<NSENSORS; s++)
        {
            printf(", cal%d=", s);
            if(nodecfg->sensors[s] == NULL)
                printf("nc");
            else
                printf("%d", nodecfg->cal[s]);
        }
        printf("\n");
        nnodes++;
    }
#endif

#ifdef DEBUG
    printf("DEBUG: end readnodecfg()\n");
#endif
   return nodecfgs;
} // readnodecfg

//---------------------------------------- printmsg
/**
  format the message received from a node and print it
  input: <mac_addr> <sensor0> <sensor1> <sensor2> <sensor3>
  output: <time> <name> <sensor0> <sensor1> <sensor2> <sensor3>
  @param nodecfg null terminated list of array of node config data
  @param msg  message received from node
  @param fn_out output file name, if NULL then use stdout
  @returns 0 on success
*/
int
printmsg(struct nodecfg_st **nodecfgs, char *msg, char *fn_out)
{
    int       retval = 0;
    int       i;
    char      *tok;
    struct    nodecfg_st *nodecfg = NULL;
    time_t    now;
    struct tm *tmnow;
    FILE      *fh;

    if(fn_out)
        fh = fopen(fn_out, "a");

    if(fh == NULL)
        fh = stdout;

    // find the nodecfg whose mac matches the message mac addr

    tok = strtok(msg, " \n");
    if(tok != NULL && *tok != '\0')
    {
        i = 0;
        while(nodecfgs[i] && nodecfg == NULL)
        {
            if(nodecfgs[i]->mac && strcmp(nodecfgs[i]->mac, tok) == 0)
                nodecfg = nodecfgs[i];
            else
                i++;
        }
    } // if tok

    now = time(NULL);
    tmnow = localtime(&now);
    fprintf(fh, "%04d%02d%02d%02d%02d%02d ", 1900+(tmnow->tm_year)
     , 1+tmnow->tm_mon, tmnow->tm_mday, tmnow->tm_hour, tmnow->tm_min
     , tmnow->tm_sec);

    if(nodecfg)
        fprintf(fh, "%s ", (nodecfg->name ? nodecfg->name : nodecfg->mac));
    else
        fprintf(fh, "%s ", tok);

    for(i=0; i<NSENSORS; i++)
    {
        if(tok && *tok != '\0')
        {
            tok = strtok(NULL, " \n");
            if(nodecfg->sensors[i] != NULL)
                fprintf(fh, "%d ", nodecfg->cal[i] + atoi(tok));
            else
                fprintf(fh, "- ");
        }
    }

    fprintf(fh, "\n");

    if(fh != stdout)
        fclose(fh);

    return retval;
} // printmsg

//---------------------------------------- readnode
/**
  read network message from node at fd and store in msg
  @param fd   file descriptor to read from
  @param msg  previously allocated storage for message, new data is
              appended to this buffer
  @returns number of bytes read, -1 on EOF
*/
int
readnode (int fd, char *msg)
{
   char buffer[MAXMSGLEN];
   int  nbytes;
   int  n;
   int  retval = 0;

   nbytes = read(fd, buffer, MAXMSGLEN);
   if(nbytes < 0)
   {
      // TODO: this is an error really, need to handle better
      retval = -1;
   }
   else if(nbytes == 0)
   {
      // EOF
      retval = -1;
   }
   else
   {
      buffer[nbytes] = '\0';
      n = MAXMSGLEN - (strlen(msg) + nbytes);
      strncat(msg, buffer, n);
      retval = nbytes;
   }

    return retval;
} // readnode

//---------------------------------------- make_socket
/**
   Creates a bound socket that can be listened
   @param port port number
   @return open socket, -1 on error
*/
int
make_socket (uint16_t port)
{
   int    sock;
   struct sockaddr_in name;

   sock = socket (PF_INET, SOCK_STREAM, 0);
   if (sock < 0)
   {
      perror ("socket");
      sock = -1;
   }
   else
   {
      name.sin_family = AF_INET;
      name.sin_port = htons (port);
      name.sin_addr.s_addr = htonl (INADDR_ANY);
      if(bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0)
      {
         perror ("bind");
         sock = -1;
      }
   }

  return sock;
} // make_socket

//---------------------------------------- usage
/** usage
  display command line options and usage
*/
void
usage(void)
{
    printf(
     "USAGE: owbmonsvr [-h] [-f] [-c <nodecfg_file>]\n"
     "-c <nodecfg_file>  file with node configurations/calibrations\n"
     "-f                 run in the foreground, otherwise we daemonize\n"
     "-h                 this help message\n"
     "-o <out_file>      store output data in this file (default is stdout)\n"
     "-m                 allow multiple instances of owbmonsvr\n"
     "-p <port>          listen on specified port, default is %d\n"
     "\n"
     "Node Config File\n"
     "  One line per monitor node, space delimited fields\n"
     "  <macaddr> <nodename> <name1>:<cal1> <name2>:<cal2> <name3>:<cal3> <name4>:<cal4>\n"
     "  eg: 18fe34f17979 furnace in:2 out:-1 ambient:0 power:0\n"
     "  The calibration fields are added to the sensor values.\n"
     "\n", PORT
    );

    return;
} // usage

//---------------------------------------- rmrunfile
/**
  remove run file
*/
void
rmrunfile(void)
{
    unlink(fn_run);
} // rmrunfile

//---------------------------------------- makerunfile
/**
  create a run file, exit if it already exists and pid is active
*/
void
makerunfile(void)
{
    FILE  *fh; 
    char  txt[MAXPIDLEN+2];
    int   mkfile = 1;
    pid_t pid;

    fh = fopen(fn_run, "r");
    if(fh != NULL)
    {
        fgets(txt, MAXPIDLEN, fh);
        fclose(fh);

        pid = atol(txt);
        if(kill(pid, 0) == 0)
        {
            mkfile = 0;
            perror("owbmonsvr already running, exiting");
            exit(1);
        }
    }

    if(mkfile)
    {
        fh = fopen(fn_run, "w+");
        if(fh != NULL)
        {
            fprintf(fh, "%ld",  (long) getpid());
            fclose(fh);
            atexit(rmrunfile);
        }
    }

    return;
} // makerunfile

//---------------------------------------- main
/**
   @param argc command line argument count
   @param argv command line arguments
   @return 0 on success
*/
int
main(int argc, char *argv[])
{
    int    sock;
    int    newsock;
    int    i;
    int    foreground = 0;
    int    portn = PORT;
    int    allowmulti = 0;
    size_t size;
    fd_set active_fd_set;
    fd_set read_fd_set;
    struct sockaddr_in clientname;
    char   msgs[255][MAXMSGLEN+1];
    char   opt;
    char   *fn_nodecfg = NULL;
    char   *fn_out = NULL;
    struct nodecfg_st **nodecfgs = NULL;
    struct nodemsg_st *nodemsg;

    fn_run = RUNFILE;

    while((opt=getopt(argc, argv, "c:fho:p:r:")) != -1)
    {
        switch(opt)
        {
            case 'c':
                fn_nodecfg = strdup(optarg);
                break;

            case 'f':
                foreground = 1;
                break;

            case 'o':
                fn_out = strdup(optarg);
                break;

            case 'p':
                portn = atoi(optarg);
                break;

            case 'r':
                fn_run = optarg;
                break;

            case 'h':
            default:
                usage();
                exit(1);
                break;
        } // switch
    } // while

    if(foreground == 0)
        daemon(1, 1);

    if(fn_nodecfg)
        nodecfgs = readnodecfg(fn_nodecfg);

   // we exit if there is another instance already running
   makerunfile();

   sock = make_socket(portn);
   if (listen(sock, LSTNBACKLOG) < 0)
   {
      perror("listen");
      exit(EXIT_FAILURE);
   }

   FD_ZERO(&active_fd_set);
   FD_SET(sock, &active_fd_set);

   while(1)
   {
      // TODO: failed select should attempt to create new socket
      read_fd_set = active_fd_set;
      if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
      {
      // TODO: handle this error without exiting
         perror ("select");
         exit (EXIT_FAILURE);
      }

      /* Service all the sockets with input pending. */
      for (i = 0; i < FD_SETSIZE; ++i)
      {
         if(FD_ISSET(i, &read_fd_set))
         {
            if(i == sock)
            {
                #ifdef DEBUG
                printf("DEBUG: select read fd %d\n", i);
                #endif

                size = sizeof(clientname);
                newsock = accept(sock, (struct sockaddr *) &clientname
                 , (socklen_t *) &size);
                if(newsock < 0)
                {
                    // TODO: handle this error without exiting
                    perror ("accept");
                    exit (EXIT_FAILURE);
                }
                msgs[newsock][0] = '\0'; 
                FD_SET (newsock, &active_fd_set);
            } // if i == sock
            else
            {
               /* Data arriving on an already-connected socket. */
               /* is appended to the message in the array */
               if(readnode(i, msgs[i]) < 0)
               {
                    /* once we get 0 bytes from the fd we know that we are
                       done reading and can process the message */
                    close(i);
                    FD_CLR(i, &active_fd_set);
                    printmsg(nodecfgs, msgs[i], fn_out);
                    memset(msgs[i], '\0', MAXMSGLEN);
                } // if readnode
            } // if i == sock else
         } // if read fd is set
      } // for
   } // while 1

   return 0;
} // main

// owbmonsvr.c
