#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_INPUT 1
#define TYPE_SECRET 2

const char *const marker_secret = "BPBPBPB";
const char *const marker_input = "TGTGTGT";

char* read_file(char *filename, long *fsize) {
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    *fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *string = (char*)malloc(*fsize + 1);
    fread(string, 1, *fsize, f);
    fclose(f);

    string[*fsize] = '\0';

    return string;
}

typedef struct {
    int *arr;
    size_t len, cap;
}vector;

vector* new_vec() {
    vector *v = (vector*)malloc(sizeof(vector));
    v->len = 0;
    v->cap = 1000;
    v->arr = (int*)malloc(sizeof(int) * v->cap);

    return v;
}

void push_vec(vector *v, int val) {
    if (v->len == v->cap) {
        v->cap *= 2;
        v->arr = (int*)realloc(v->arr, sizeof(int) * v->cap);
    }
    v->arr[v->len++] = val;
}

void print_brs(char *filename, vector *brs) {
    FILE *fp = fopen(filename, "w");
    size_t i;

    if (brs->len == 0) {
        fprintf(fp, "0,0");
    } else {
        for (i = 0; i < brs->len ; i++) {
            if (i == 0) {
                fprintf(fp, "%d", brs->arr[i]);
            } else {
                fprintf(fp, ",%d", brs->arr[i]);
            }
        }
    }
    fclose(fp);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("no file specified\n");
        return 1;
    }
    long fsize;
    char *in_buf = read_file(argv[1], &fsize);
    char *out_buf = (char*)malloc(fsize + 1);
    size_t out_size = 0;
    char *last = in_buf;

    char *match, *match_secret, *match_input;
    int match_type = 0, current_type = 0;
    vector *brs, *brs_secret, *brs_input;

    int stack = 0;

    char output_filename[100] = "";
    brs_secret = new_vec();
    brs_input = new_vec();

    while (1) {
        match_secret = strstr(last, marker_secret);
        match_input = strstr(last, marker_input);

        if (match_secret && match_input) {
            match = match_secret < match_input ? match_secret : match_input;
        } else if (match_secret) {
            match = match_secret;
        } else if (match_input) {
            match = match_input;
        } else {
            break;
        }

        match_type = match == match_secret ? TYPE_SECRET : TYPE_INPUT;
        brs = match == match_secret ? brs_secret : brs_input;

        memcpy(out_buf + out_size, last, (match - last));
        out_size += (match - last);
        last = match + sizeof(marker_secret);

        if (current_type && current_type != match_type) {
            printf("Mixed INPUT/SECRET\n");
            return 1;
        }

        if (*(last-1) == '{') {
            if (stack == 0) {
                current_type = match_type;
                push_vec(brs, out_size);
            }
            stack += 1;
        } else if (*(last-1) == '}') {
            stack -= 1;
            if (stack == 0) {
                current_type = 0;
                push_vec(brs, out_size - 1);
            }
        } else {
            printf("Bad close/open char : %c\n", *last);
            return 1;
        }

        if (stack < 0) {
            printf("stack is less than 0\n");
            return 1;
        }
    }

    if (strlen(last) > 0) {
        memcpy(out_buf + out_size, last, strlen(last));
        out_size += strlen(last);
        out_buf[out_size] = '\0';
    }

    strcat(output_filename, argv[1]);
    strcat(output_filename, ".nomarkers");
    FILE *f = fopen(output_filename, "w");
    fwrite(out_buf, 1, out_size, f);
    fclose(f);

    print_brs("brs_input", brs_input);
    print_brs("brs_secret", brs_secret);

}