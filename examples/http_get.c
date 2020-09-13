/* http_get - fetch the contents of google.com and print it to stdout */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "xsocket.h"

#define HTTP_REQUEST_SIZE 1024
#define HTTP_RESPONSE_SIZE 100 * 1024

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

static int on_http_headers(
    const char *buf, int len, int chunk_offset, int chunk_len)
{
  return strstr(buf + chunk_offset, "\r\n\r\n") != NULL;
}

static int http_get(const char *hostname, const char *uri, char **response)
{
  int error = 0;
  socket_t sock;
  struct addrinfo ai_hints, *ai_result = NULL, *ai_cur;
  char *addr_str = NULL;
  char request_buf[HTTP_REQUEST_SIZE];
  char *buf;
  size_t len;

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    error = socket_error;
    goto error_return;
  }

  memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = AF_INET;
  ai_hints.ai_socktype = SOCK_STREAM;
  ai_hints.ai_protocol = IPPROTO_TCP;

  error = getaddrinfo(hostname, "80", &ai_hints, &ai_result);
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

  snprintf(request_buf,
           sizeof(request_buf),
           "GET %s HTTP/1.1\r\n\r\n",
           uri);
  len = strlen(request_buf);

  error = (int)send_n(sock, request_buf, (int)len, 0);
  if (error < len) {
    error = socket_error;
    goto error_return;
  }

  buf = malloc(HTTP_RESPONSE_SIZE);
  if (buf == NULL) {
    error = errno;
    goto error_return;
  }

  /* buf is expected to be null-terminated in on_http_headers */
  memset(buf, '\0', HTTP_RESPONSE_SIZE);

  error = (int)recv_n(sock, buf, HTTP_RESPONSE_SIZE, 0, on_http_headers);
  if (error <= 0) {
    error = socket_error;
    goto error_return;
  }

  *response = buf;

  close_socket_nicely(sock);
  free(addr_str);
  return 0;

error_return:
  freeaddrinfo(ai_result);
  free(addr_str);
  close_socket_nicely(sock);
  return error;
}

int main()
{
  char *response;
  int error;

  error = socket_init();
  if (error != 0) {
    fprintf(stderr, "Error: %s\n", error_str(error));
    return 1;
  }

  error = http_get("google.com", "/", &response);
  if (error != 0) {
    fprintf(stderr, "Error: %s\n", error_str(error));
    return 1;
  }

  printf("%s\n", response);
  free(response);

  socket_cleanup();

  return 0;
}
