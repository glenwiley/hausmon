/**
   @file hausmonsvr.c
   @author Glen Wiley <glen.wiley@gmail.com>
   @brief hausmon network server that accepts data form hausmon nodes
   @details network server that accepts multiple concurrent clients
            intended to provide reporting interface to hausmon nodes
            runs continuously, exits != 0 if cant create a socket to listen on

TODO: daemonize
TODO: proper syslog style error logging
TODO: read node config file to get name and calibration data
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

#define PORT       5555
#define MAXMSGLEN  512
#define MAXCFGLINE 255
// backlog for listen for server socket
#define LSTNBACKLOG 5

// used to store calibration data for a specific node
// values in calibration fields are added to values received from node
struct nodecfg_st
{
   char *mac;
   char *name;
   int  cal1;
   int  cal2;
   int  cal3;
   int  cal4;
};

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
   char   txt[MAXCFGLINE+1];
   char   *tok;
   struct nodecfg_st **nodecfgs = NULL;
   struct nodecfg_st *node;

   f = fopen(fn, "r");
   if(f != NULL)
   {
      while(fgets(txt, MAXCFGLINE, f) != NULL)
      {
         if(txt[0] != '#' && txt[0] != '\n')
         {
            // TODO: bail on malloc error
            node = (struct nodecfg_st *) malloc(sizeof(struct nodecfg_st));

            tok  = strtok(txt, " \n");
            ntok = 0;
            while(tok != NULL)
            {
               if(ntok == 0)
                  node->mac = strdup(tok);
               else if(ntok == 1)
                  node->name = strdup(tok);
               else if(ntok == 2)
                  node->cal1 = atoi(tok);
               else if(ntok == 3)
                  node->cal2 = atoi(tok);
               else if(ntok == 4)
                  node->cal3 = atoi(tok);
               else if(ntok == 5)
                  node->cal4 = atoi(tok);

               ntok++;
               tok = strtok(NULL, " \n");
            } // while tok

            nodecfgs = (struct nodecfg_st **) realloc(nodecfgs
             , nnodes * sizeof(struct nodecfg_st *));
            nodecfgs[nnodes] = node;
         } // if txt
      } // while
      fclose(f);
   }

   return nodecfgs;
} // readnodecfg

//---------------------------------------- getnodecfg
/**
  search the list of node configs for a matching mac

*/
struct nodecfg_st *
getnodecfg()
{
} // getnodecfg

//---------------------------------------- printmsg
/**
  format the message received from a node and print it
  @param msg  message received from node
  @returns 0 on success
*/
int
printmsg(char *msg)
{
   int retval = 0;
   return retval;
} // printmsg

//---------------------------------------- readnode
/**
  read network message from node at fd and store in msg
  @param fd   file descriptor to read from
  @param msg  previously allocate storage for message
  @returns number of bytes read, -1 on EOF
*/
int
readnode (int fd, char *msg)
{
   char buffer[MAXMSGLEN];
   int  nbytes;
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
      strcat(msg, buffer);
      retval = nbytes;
   }

    return retval;
} // readnode

//---------------------------------------- make_socket
/**
   Creates a bound socket that can be listended
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

//---------------------------------------- main
/**
   @param argc command line argument count
   @param argv command line arguments
   @return 0 on success
*/
int
main (void)
{
   int    sock;
   int    newsock;
   int    i;
   size_t size;
   fd_set active_fd_set;
   fd_set read_fd_set;
   struct sockaddr_in clientname;
   char   msgs[255][MAXMSGLEN+1];

   sock = make_socket(PORT);
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
               size = sizeof(clientname);
               newsock = accept(sock, (struct sockaddr *) &clientname
                , (socklen_t *) &size);
               if(newsock < 0)
               {
                 // TODO: handle this error without exiting
                  perror ("accept");
                  exit (EXIT_FAILURE);
               }
//               fprintf (stderr,
//                         "Server: connect from host %s, port %hd.\n",
//                         inet_ntoa (clientname.sin_addr),
//                         ntohs (clientname.sin_port));
                msgs[newsock][0] = '\0'; 
                FD_SET (newsock, &active_fd_set);
            } // if i == sock
            else
            {
               /* Data arriving on an already-connected socket. */
               if(readnode(i, msgs[i]) < 0)
               {
                  parsemsg(msgs[i]);
                  close(i);
                  FD_CLR(i, &active_fd_set);
               }
            } // if i == sock else
         } // if read fd is set
      } // for
   } // while 1

   return 0;
} // main

// hausmonsvr.c
