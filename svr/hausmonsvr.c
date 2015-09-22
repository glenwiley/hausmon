/**
   @file hausmonsvr.c
   @author Glen Wiley <glen.wiley@gmail.com>
   @brief hausmon network server that accepts data form hausmon nodes
   @details network server that accepts multiple concurrent clients
            intended to provide reporting interface to hausmon nodes
            runs continuously, exits != 0 if cant create a socket to listen on

TODO: daemonize
TODO: proper syslog style error logging
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT    5555
#define MAXMSG  512
// backlog for listen for server socket
#define LSTNBACKLOG 5

int
read_from_client (int filedes)
{
  char buffer[MAXMSG];
  int nbytes;

  nbytes = read (filedes, buffer, MAXMSG);
  if (nbytes < 0)
    {
      /* Read error. */
      perror ("read");
      exit (EXIT_FAILURE);
    }
  else if (nbytes == 0)
    /* End-of-file. */
    return -1;
  else
    {
      /* Data read. */
      fprintf (stderr, "Server: got message: `%s'\n", buffer);
      return 0;
    }
}

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
               newsock = accept(sock, (struct sockaddr *) &clientname, &size);
               if(newsock < 0)
               {
                  perror ("accept");
                  exit (EXIT_FAILURE);
               }
               fprintf (stderr,
                         "Server: connect from host %s, port %hd.\n",
                         inet_ntoa (clientname.sin_addr),
                         ntohs (clientname.sin_port));
                FD_SET (new, &active_fd_set);
            } // if i == sock
            else
            {
                /* Data arriving on an already-connected socket. */
                if (read_from_client (i) < 0)
                  {
                    close (i);
                    FD_CLR (i, &active_fd_set);
                  }
            } // if i == sock else
         } // if read fd is set
    } // while 1

    return 0;
} // main

// hausmonsvr.c
