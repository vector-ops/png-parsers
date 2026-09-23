#include "crc32.c"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CRC32_FAST

typedef struct {
  uint32_t len;
  char type[5];
  unsigned char *data;
  uint32_t crc;
} Chunk;

int get_signature(FILE *f, unsigned char *dst, size_t off);
int is_png(const unsigned char *hdr);
int read_chunk(FILE *f, Chunk chunk[], size_t off);
int test_crc(Chunk c);
void print_chunk(Chunk c);

int main(int argc, char *argv[]) {

  if (argc < 2) {
    printf("atleast 1 png file is required\n");
    exit(1);
  }

  char *filename = argv[1];

  printf("filename: %s\n", filename);

  FILE *f = fopen(filename, "r");

  if (f == NULL) {
    perror("failed to open file");
    exit(1);
  }

  size_t offset = 0;

  unsigned char *f_hdr = malloc(8 * sizeof(unsigned char));

  if (get_signature(f, f_hdr, offset) == -1) {
    printf("failed to read file header\n");
    exit(1);
  }

  if (f_hdr == NULL) {
    perror("failed to read header");
    exit(1);
  }

  if (is_png(f_hdr) == -1) {
    printf("file is not PNG: %s\n", (char *)f_hdr);
    exit(1);
  }
  printf("test file type: PASS\n");

  Chunk *c = malloc(sizeof(Chunk));

  size_t n = read_chunk(f, c, offset);
  if (n == -1) {
    printf("failed to read chunk\n");
    exit(1);
  }

  while (strcmp(c->type, "IEND") != 0) {

    offset += n;

    print_chunk(*c);

    free(c->data);
    free(c);
    c = malloc(sizeof(Chunk));
    n = read_chunk(f, c, offset);
    if (n == -1) {
      printf("failed to read chunk\n");
      exit(1);
    }
  }

  free(c->data);
  free(c);
  free(f_hdr);
  fclose(f);

  return 0;
}

int test_crc(Chunk c) {
  // crc is computed over the chunk type and chunk data

  unsigned char *data = (unsigned char *)malloc(c.len + 4);

  memcpy(data, c.type, 4);
  memcpy(data + 4, c.data, c.len);

  unsigned int crc = crc32(data, c.len + 4);

  free(data);

  if (crc != (unsigned int)c.crc) {
    return 0;
  }

  return 1;
}

int read_chunk(FILE *f, Chunk *c, size_t off) {
  if (c == NULL) {
    printf("received uninitialized chunk\n");
    return -1;
  }

  unsigned char *buf = (unsigned char *)malloc(8 * sizeof(unsigned char));

  size_t n = fread(buf, sizeof(unsigned char), 8, f);
  if (n != 8) {
    free(buf);
    return -1;
  }

  // read Big Endian
  c->len = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];

  memcpy(c->type, buf + 4, 4);
  c->type[4] = '\0';

  unsigned char *tmp =
      (unsigned char *)realloc(buf, (c->len + 4) * sizeof(unsigned char));
  if (tmp) {
    buf = tmp;
  }

  n = fread(buf, sizeof(unsigned char), (c->len + 4), f);
  if (n != c->len + 4) {
    free(buf);
    return -1;
  }

  c->data = malloc(c->len * sizeof(unsigned char));

  memcpy(c->data, buf, c->len);

  // Big Endian copy
  c->crc = ((uint32_t)buf[c->len] << 24) | ((uint32_t)buf[c->len + 1] << 16) |
           ((uint32_t)buf[c->len + 2] << 8) | (uint32_t)buf[c->len + 3];

  free(buf);

  return 4 + 4 + c->len + 4;
}

int get_signature(FILE *f, unsigned char *dst, size_t off) {

  size_t c = fread(dst, 1, sizeof(dst), f);
  if (c == 0) {
    return -1;
  }

  return c;
}

int is_png(const unsigned char *hdr) {
  if (sizeof(hdr) < 3) {
    return -1;
  }

  char *hdr_s = malloc(4 * sizeof(char));
  if (sizeof(hdr) == 3) {
    memcpy(hdr_s, hdr, 3);
  } else {
    strncpy(hdr_s, (char *)hdr + 1, 3);
  }
  hdr_s[3] = '\0';

  free(hdr_s);

  return 0;
}

void print_chunk(Chunk c) {

  int crc_pass = test_crc(c);

  printf("chunk %s\nlen: %d\ntype: %s\ncrc: %d\n", c.type, c.len, c.type,
         c.crc);

  if (!crc_pass) {
    printf("intigrity test: FAIL\n\n");
  } else {
    printf("intigrity test: PASS\n\n");
  }

  // print chunk data
  // for (size_t i = 0; i < c.len; i++) {
  //   printf(" %02X", c.data[i]);
  // }
  // printf("\n");
}
