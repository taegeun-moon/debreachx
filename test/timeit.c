#include "zlib.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#define TRIALS 10
#define WARMUPS 10
#define BUFLEN 16384

#define local

void *myalloc(q, n, m)
    void *q;
    unsigned n, m;
{
    q = Z_NULL;
    return calloc(n, m);
}

void myfree(q, p)
    void *q, *p;
{
    q = Z_NULL;
    free(p);
}


typedef struct gzFile_st {
	// We aren't writing out to a file
    // FILE *file;
    int write;
    int err;
    char *msg;
    z_stream strm;
} *gzFilet;

const char *gzerrort(gz, err)
    gzFilet gz;
    int *err;
{
    *err = gz->err;
    return gz->msg;
}

int gzcloset(gz)
    gzFilet gz;
{
    z_stream *strm;
    unsigned char out[BUFLEN];

    if (gz == NULL)
        return Z_STREAM_ERROR;
    strm = &(gz->strm);
    if (gz->write) {
        strm->next_in = Z_NULL;
        strm->avail_in = 0;
        do {
            strm->next_out = out;
            strm->avail_out = BUFLEN;
            (void)deflate(strm, Z_FINISH);
        } while (strm->avail_out == 0);
		// Don't want to free the state
        // debreachEnd(strm);
    }
    //fclose(gz->file);
    //free(gz);
    return Z_OK;
}

gzFilet gz_opent(mode)
    const char *mode;
{
    gzFilet gz;
    int ret;

    gz = malloc(sizeof(struct gzFile_st));
    if (gz == NULL)
        return NULL;
    gz->write = strchr(mode, 'w') != NULL;
    gz->strm.zalloc = myalloc;
    gz->strm.zfree = myfree;
    gz->strm.opaque = Z_NULL;
    if (gz->write)
        ret = deflateInit2(&(gz->strm), -1, 8, 15 + 16, 8, 0);
    else {
        gz->strm.next_in = 0;
        gz->strm.avail_in = Z_NULL;
        ret = inflateInit2(&(gz->strm), 15 + 16);
    }
    if (ret != Z_OK) {
        free(gz);
        return NULL;
    }
	// Not writing out to file
    // gz->file = path == NULL ? fdopen(fd, gz->write ? "wb" : "rb") :
    //                          fopen(path, gz->write ? "wb" : "rb");
    //if (gz->file == NULL) {
    //    gz->write ? debreachEnd(&(gz->strm)) : inflateEnd(&(gz->strm));
    //    free(gz);
    //    return NULL;
    //}
    gz->err = 0;
    gz->msg = "";
    return gz;
}

int gzwritet(gz, buf, len)
    gzFilet gz;
    const void *buf;
    unsigned len;
{
    z_stream *strm;
    unsigned char out[BUFLEN];
    if (gz == NULL || !gz->write)
        return 0;
    strm = &(gz->strm);
    strm->next_in = (void *)buf;
    strm->avail_in = len;
    do {
        strm->next_out = out;
        strm->avail_out = BUFLEN;
		(void)deflate(strm, Z_NO_FLUSH);
    } while (strm->avail_out == 0);
    return len;
}

void gz_compress(in, out, brs_input, brs_input_len, brs_secret, brs_secret_len)
    FILE   *in;
    gzFilet out;
    int   *brs_input;
    int    brs_input_len;
    int   *brs_secret;
    int    brs_secret_len;
{
    local char buf[BUFLEN];
    int len;
    int err;

    append_all_brs(&(out->strm), brs_input, brs_input_len, brs_secret, brs_secret_len);

    for (;;) {
        len = (int)fread(buf, 1, sizeof(buf), in);
        if (ferror(in)) {
            perror("fread");
            exit(1);
        }
        if (len == 0) break;

        if (gzwritet(out, buf, (unsigned)len, NULL) != len) { 
			printf("write error\n");
			exit(1);	
		}
    }
    //fclose(in);
    if (gzcloset(out) != Z_OK) printf("failed gzclose\n");
}



int isNumber(str)
    char *str;
{
    char *temp = str;
    while (*temp != '\0') {
      if (isdigit((int)*temp) == 0)
	return 0;
      temp++;
    }
    return 1;
}

int charCount(str, c)
   char *str;
   char c;
{
   int ct = 0;
   char *temp = str;
   while (*temp != '\0') {
	   if (*temp == c)
		   ct++;
	   temp++;
   }
   return ct;
}

char* read_br_file(char *filename) {
	/* declare a file pointer */
	FILE    *infile;
	char    *buffer;
	long    numbytes;

	/* open an existing file for reading */
	infile = fopen(filename, "r");

	/* quit if the file does not exist */
	if(infile == NULL)
		return NULL;

	/* Get the number of bytes */
	fseek(infile, 0L, SEEK_END);
	numbytes = ftell(infile);

	/* reset the file position indicator to 
	   the beginning of the file */
	fseek(infile, 0L, SEEK_SET);	

	/* grab sufficient memory for the 
	   buffer to hold the text */
	buffer = (char*)calloc(numbytes + 1, sizeof(char));	

	/* memory error */
	if(buffer == NULL)
		return NULL;

	/* copy all the text into the buffer */
	fread(buffer, sizeof(char), numbytes, infile);
	buffer[numbytes] = '\0';
	fclose(infile);

	/* confirm we have read the file by
	   outputing it to the console */
	//printf("The file called test.dat contains this text\n\n%s", buffer);
	return buffer;
}

int *parseBRS(brs_char, len)
    char *brs_char;
    int *len;
{
    int *brs;
    char *tmp;
    *len = charCount(brs_char, ',') + 1;
    if (*len == 1) *len = 0;
    int i = 0;
    brs = (int*)malloc(sizeof(int)*(*len + 2));

    tmp = strtok(brs_char, ",");
    while(tmp != NULL) {
        brs[i++] = atoi(tmp);
        tmp = strtok(NULL, ",");   
    }
    brs[i] = brs[i+1] = 0;

    return brs;
}


int main(argc, argv) 
	int argc;
	char *argv[];
{
	int n;
	char *buf, *orig_buf;
    char *brs_secret_str = NULL, *brs_input_str = NULL;
    int *brs_secret = NULL, *brs_input = NULL;
    int brs_secret_len = 0, brs_input_len = 0;
	if (n % 2 != 0) {
		printf("Invalid br file: odd number of byte values supplied.\n");
		printf("Argument: %s\n", buf);
		exit(1);
	}


    brs_input_str = read_br_file("brs_input");
    brs_secret_str = read_br_file("brs_secret");
    brs_input = parseBRS(brs_input_str, &brs_input_len);
    brs_secret = parseBRS(brs_secret_str, &brs_secret_len);
	// + 2 for the double null terminator
	free(orig_buf);
	// Compression data structures that we only want to allocate once
	FILE *in;
	in = fopen(argv[1], "rb");
	gzFilet out = gz_opent("w");

	int t;
	clock_t start, diff;

	for (t = 0; t < WARMUPS; t++) {
    	fseek(in, 0, SEEK_SET);
    	deflateReset(&(out->strm));
    	gz_compress(in, out,  brs_input, brs_input_len, brs_secret, brs_secret_len);		
	}

	for (t = 0; t < TRIALS; t++) {
		struct timespec tstart={0,0}, tend={0,0};
		clock_gettime(CLOCK_MONOTONIC, &tstart);

    	fseek(in, 0, SEEK_SET);
    	deflateReset(&(out->strm));
    	gz_compress(in, out,  brs_input, brs_input_len, brs_secret, brs_secret_len);

		clock_gettime(CLOCK_MONOTONIC, &tend);
		printf("%.9f, ",
			((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
			((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));
	}

	// Cleanup
	deflateEnd(&(out->strm));
	free(out);
	fclose(in);
	free(brs_input);
	free(brs_secret);
	return 0;
}
