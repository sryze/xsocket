/* http_get - fetch the contents of google.com and print it to stdout */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "xsocket.h"

#define HTTP_REQUEST_SIZE 1024
#define HTTP_RESPONSE_SIZE 100 * 1024
#define HTTP_DEFAULT_PORT "80"

#ifdef _WIN32

#define ERROR_STR_BUFFER_SIZE 1024

static char *error_str(int error_code)
{
  static char static_buf[ERROR_STR_BUFFER_SIZE];

  (void)FormatMessageA(
    FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_MAX_WIDTH_MASK,
    NULL,
    error_code,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPSTR)static_buf,
    ERROR_STR_BUFFER_SIZE,
    NULL);
  return static_buf;
}

#else

#include <string.h>

char *error_str(int error_code)
{
  return strerror(error_code);
}

#endif

static int http_get(const char *hostname,
                    const char *port,
                    const char *uri,
                    char **request,
                    char **response)
{
  int error = 0;
  socket_t sock;
  struct addrinfo ai_hints, *ai_result = NULL, *ai_cur;
  char *addr_str = NULL;
  char *request_buf = NULL;
  char *response_buf = NULL;
  size_t len;
  char port_spec[7] = "";

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    error = socket_error;
    goto error_return;
  }

  memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = AF_INET;
  ai_hints.ai_socktype = SOCK_STREAM;
  ai_hints.ai_protocol = IPPROTO_TCP;

  error = getaddrinfo(hostname, port, &ai_hints, &ai_result);
  if (error != 0) {
    goto error_return;
  }

  for (ai_cur = ai_result; ai_cur != NULL; ai_cur = ai_cur->ai_next) {
    error = connect(sock,
      (struct sockaddr *)ai_cur->ai_addr,
      (int)ai_cur->ai_addrlen);
    if (error == 0) {
      addr_str = strdup(
        inet_ntoa(((struct sockaddr_in *)ai_cur->ai_addr)->sin_addr));
      break;
    }
  }

  if (ai_cur == NULL) {
    error = socket_error;
    goto error_return;
  }

  freeaddrinfo(ai_result);
  ai_result = NULL;

  if (strcmp(port, HTTP_DEFAULT_PORT) != 0) {
    snprintf(port_spec, sizeof(port_spec), ":%s", port);
  }

  request_buf = calloc(1, HTTP_REQUEST_SIZE);
  if (request_buf == NULL) {
    error = errno;
    goto error_return;
  }

  snprintf(request_buf,
           HTTP_REQUEST_SIZE,
           "GET %s HTTP/1.0\r\n"
           "Host: %s%s\r\n"
           "Accept: */*\r\n"
           "\r\n",
           uri,
           hostname,
           port_spec);
  len = strlen(request_buf);

  error = (int)send_n(sock, request_buf, (int)len, 0);
  if (error < len) {
    error = socket_error;
    goto error_return;
  }

  response_buf = calloc(1, HTTP_RESPONSE_SIZE);
  if (response_buf == NULL) {
    error = errno;
    goto error_return;
  }

  error = (int)recv_n(sock, response_buf, HTTP_RESPONSE_SIZE, 0, NULL);
  if (error <= 0) {
    error = socket_error;
    goto error_return;
  }

  *request = request_buf;
  *response = response_buf;

  close_socket_nicely(sock);
  free(addr_str);

  return 0;

error_return:
  free(response_buf);
  free(request_buf);
  freeaddrinfo(ai_result);
  free(addr_str);
  close_socket_nicely(sock);

  return error;
}

int main(int argc, char **argv)
{
  char *hostname;
  char *port;
  char *path;
  char *request;
  char *response;
  int error;

  if (argc <= 1) {
    fprintf(stderr,
      "Usage: %s <hostname> <port> <path>\n\n"
      "Examples:\n\n"
      "%s google.com 80 /\n"
      "%s youtube.com 80 /feed/trending\n\n",
      argv[0], argv[0], argv[0]);
    return 1;
  }

  hostname = argv[1];
  port = argv[2];
  path = argv[3];

  error = socket_init();
  if (error != 0) {
    fprintf(stderr, "Error: %s\n", error_str(error));
    return 1;
  }

  error = http_get(hostname, port, path, &request, &response);
  if (error != 0) {
    fprintf(stderr, "Error: %s\n", error_str(error));
    return 1;
  }

  puts(request);
  free(request);

  puts(response);
  free(response);

  socket_cleanup();

  return 0;
}
