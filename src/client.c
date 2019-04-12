#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "lib.h"

#define BUFSIZE 4096 // max number of bytes we can get at once

/**
 * Struct to hold all three pieces of a URL
 */
typedef struct urlinfo_t
{
  char *hostname;
  char *port;
  char *path;
} urlinfo_t;

/**
 * Tokenize the given URL into hostname, path, and port.
 *
 * url: The input URL to parse.
 *
 * Store hostname, path, and port in a urlinfo_t struct and return the struct.
*/
urlinfo_t *parse_url(char *url)
{
  // copy the input URL so as not to mutate the original
  char *hostname = strdup(url);
  char *port;
  char *path;

  urlinfo_t *urlinfo = malloc(sizeof(urlinfo_t));

  // ex.) localhost:3490/d20 and https://www.google.com:80/
  // GET /path HTTP/1.1
  // Host: hostname:port
  // Connection: close

  // check if url has http at the beginning
  // returns a pointer to the first occurrence in haystack
  char *double_slashes = strstr(hostname, "//");
  int url_len = strlen(hostname);

  if (double_slashes)
  {
    // has "//" (assumed protocol prefix)
    // index for second /
    int index = (int)(double_slashes - hostname) + 1;
    int length_copy = url_len - (index + 1);

    // removes protocol info
    char *remove_protocol = malloc(sizeof(char) * (length_copy + 1));
    memcpy(remove_protocol, &hostname[index + 1], (length_copy + 1));
    free(hostname);

    char host2[url_len];
    char port2[url_len];
    char path2[url_len];

    sscanf(remove_protocol, "%[^:]:%[^/]%s", host2, port2, path2);

    urlinfo->hostname = strdup(host2);
    urlinfo->port = strdup(port2);
    urlinfo->path = strdup(path2);

    free(remove_protocol);
  }
  else
  {
    // no "//"
    char host2[url_len];
    char port2[url_len];
    char path2[url_len];

    sscanf(hostname, "%[^:]:%[^/]%s", host2, port2, path2);

    urlinfo->hostname = strdup(host2);
    urlinfo->port = strdup(port2);
    urlinfo->path = strdup(path2);

    free(hostname);
  }

  return urlinfo;
}

/**
 * Constructs and sends an HTTP request
 *
 * fd:       The file descriptor of the connection.
 * hostname: The hostname string.
 * port:     The port string.
 * path:     The path string.
 *
 * Return the value from the send() function.
*/
int send_request(int fd, char *hostname, char *port, char *path)
{
  const int max_request_size = 16384;
  char request[max_request_size];
  int rv;

  // GET /path HTTP/1.1
  // Host: hostname:port
  // Connection: close

  int request_length = sprintf(request,
                               "GET %s HTTP/1.1\n"
                               "Host: %s:%s\n"
                               "Connection: close\n"
                               "\n",
                               path,
                               hostname,
                               port);

  printf("SEND REQUEST-----------\n%s\n-----------------------\n", request);

  rv = send(fd, request, request_length, 0);

  // number of bytes sent
  return rv;
}

int main(int argc, char *argv[])
{
  int sockfd, numbytes;
  char buf[BUFSIZE];

  if (argc != 2)
  {
    fprintf(stderr, "usage: client HOSTNAME:PORT/PATH\n");
    exit(1);
  }

  // 1. Parse the input URL
  struct urlinfo_t *url = parse_url(argv[1]);

  // 2. Initialize a socket by calling the `get_socket` function from lib.c
  // int get_socket(char *hostname, char *port)
  sockfd = get_socket(url->hostname, url->port);

  printf("hostname: %s\n", url->hostname);
  printf("port: %s\n", url->port);
  printf("path: %s\n", url->path);

  // 3. Call `send_request` to construct the request and send it
  // int send_request(int fd, char *hostname, char *port, char *path)
  numbytes = send_request(sockfd, url->hostname, url->port, url->path);

  printf("numbytes: %d\n", numbytes);

  // 4. Call `recv` in a loop until there is no more data to receive from the server.
  // Print the received response to stdout.
  recv(sockfd, buf, BUFSIZE, 0);
  fprintf(stdout, "%s\n", buf);

  // 5. Clean up any allocated memory and open file descriptors.
  free(url->hostname);
  free(url->path);
  free(url->port);
  free(url);

  close(sockfd);

  return 0;
}
