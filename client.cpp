/* UDP client in the internet domain */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sockets.h"

#define CACHE_CMD_NEW     0x00
#define CACHE_CMD_GET     0x01
#define CACHE_CMD_REMOVE  0x02
#define CACHE_RESP_ERR    0x80
#define CACHE_RESP_OK     0x81

#define KEY_LEN 32
#define MAX_VAL_LEN 512
typedef struct {
  u_char version, type;
  u_short timeout;
  u_char key[KEY_LEN];
  u_char val[MAX_VAL_LEN];
} CACHE_PACKET;

void error(const char *);
int main(int argc, char *argv[])
{
  int sock, n;
  socklen_t server_addrlen, from_addrlen;
  struct sockaddr_in server, from;
  struct hostent *hp;
  unsigned short port;
  char buffer[256];

  if (argc != 4) {
    printf("Usage: server port <'new'|'get'|'remove'>\n");
    exit(1);
  }
  sock= socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) error("socket");

  server.sin_family = AF_INET;
  hp = gethostbyname(argv[1]);
  if (hp==0) error("Unknown host");

  port = atoi(argv[2]);
  if ( port < 0 || port > 65535 )
  {
    perror("port range error");
  }

  bcopy((char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length);
  server.sin_port = htons(port);
  server_addrlen=sizeof(server);

  CACHE_PACKET packet;
  packet.version = 1;
  int packet_len = sizeof(packet);
  if ( strncmp(argv[3], "new", 4) == 0 )
  {
    packet.type = CACHE_CMD_NEW;
    packet.timeout = 500; // seconds
    strncpy((char*)packet.key, "testkey", KEY_LEN);
    strncpy((char*)packet.val, "testvalue", MAX_VAL_LEN);
  }
  else if ( strncmp(argv[3], "get", 4) == 0 )
  {
    packet.type = CACHE_CMD_GET;
    packet.timeout = 0;
    strncpy((char*)packet.key, "testkey", KEY_LEN);
    memset(&packet.val, 0, MAX_VAL_LEN);
  }
  else if ( strncmp(argv[3], "remove", 7) == 0 )
  {
    packet.type = CACHE_CMD_REMOVE;
    packet.timeout = 0;
    strncpy((char*)packet.key, "testkey", KEY_LEN);
    memset(&packet.val, 0, MAX_VAL_LEN);
  }
  else { error("Unimplemented"); }

  // now we want to send our packet
  n=sendto(sock, (char *)&packet, packet_len, 0, (const struct sockaddr *)&server, server_addrlen);
  if (n < 0) error("Sendto");
  printf("Sent %d bytes.\n", n);

  // we will only get something back if we asked for a 'get'
  if ( packet.type  == CACHE_CMD_GET )
  {
    printf("Waiting for GET packet response\n");
    printf("recvfrom(key:%s, val:%s, type:%d, maxlen:%d)\n", packet.key, packet.val, packet.type, packet_len);
    memset(&from, 0, sizeof(from));
    from_addrlen = sizeof(from);
    n = recvfrom(sock, (char *)&packet, packet_len, 0, (struct sockaddr *)&from, &from_addrlen);
    printf("Recieved %d bytes\n", n);
    if (n < 0) error("recvfrom");

    if ( n >= (packet_len-MAX_VAL_LEN) )
    {
      printf("Packet contents:\n");
      printf("packet.type: %s\n", packet.type == CACHE_RESP_ERR ? "error" : packet.type == CACHE_RESP_OK ? "ok" : "unknown" );
      printf("packet.key: '%s'\n", packet.key);
      printf("packet.val: '%s'\n", packet.val);
    }
  }
  close(sock);
  return 0;
}

void error(const char *msg)
{
  perror(msg);
  exit(0);
}