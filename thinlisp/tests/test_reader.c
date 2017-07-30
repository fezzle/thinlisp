#include <stdio.h>
#include <string.h>
#include "reader.h"
#include "tests/minunit.h"

int tests_run = 0;

/**
 Forward declarations of expected output of reader
 */
char result_of_sample1_lisp[];
char result_of_sample2_lisp[];

typedef char *(*TEST_FN)(char *, char *);

/**
 * A structure which contains a FILE and flags to alter the behaviour of
 * mygetc
 */
struct file_with_eof_flag {
  FILE *file;
  char characters_to_read;
};

char mygetc(void *streamobj_void) {
  struct file_with_eof_flag *streamobj = (
    struct file_with_eof_flag*)streamobj_void;
  if (streamobj->characters_to_read == 0) {
    return -1;
  }
  if (streamobj->characters_to_read > 0) {
    streamobj->characters_to_read--;
  }
  int c = fgetc(streamobj->file);
  if (c == EOF) {
    return -1;
  } else {
    return (char)c;
  }
}

char myputc(void *streamobj, char c) {
  static char lastchar = 0;
  if (lastchar == '\n') {
    printf("***   ");
  }
  printf("%c", c);
  fflush(stdout);
  lastchar = c;
  return c;
}

/**
 * Verifies reader_read does not crash
 */
static char * test_reader_result(
    char *test_lisp_file, char *expected_output) {
  struct file_with_eof_flag streamobj;
  streamobj.file = fopen(test_lisp_file, "rb");
  streamobj.characters_to_read = -1;

  BISTACK *bs = bistack_new(1<<14);
  bistack_pushdir(bs, BS_BACKWARD);
  ENVIRONMENT *environment = environment_new(bs);
  READER *reader = reader_new(environment);
  reader_set_getc(reader, mygetc, &streamobj);
  bool res = reader_read(reader);
  mu_assert("reader should complete", res);
  mu_assert("reader->is_completed should be true",
    reader->is_completed);
  mu_assert("root cell type should be list",
    reader->cell->header.List.type == AST_LIST);
  return 0;
}

/**
 * Verifies reader_pprint can be invoked successfully.
 */
static char * test_reader_pprint(
    char *test_lisp_file, char *expected_output) {
  struct file_with_eof_flag streamobj;
  streamobj.file = fopen(test_lisp_file, "rb");
  streamobj.characters_to_read = -1;

  BISTACK *bs = bistack_new(1<<14);
  bistack_pushdir(bs, BS_BACKWARD);
  ENVIRONMENT *environment = environment_new(bs);
  READER *reader = reader_new(environment);
  reader_set_getc(reader, mygetc, &streamobj);
  reader_set_putc(reader, myputc, NULL);
  bool res = reader_read(reader);
  mu_assert("reader should complete", res);
  reader_pprint(reader);
  mu_assert("root cell type should be list",
    reader->cell->header.List.type == AST_LIST);
  return 0;
}


/**
 * Verifies reader_read can handle EOFs and continueing by only allowing
 * one character to be read at a time.
 */
static char * test_reader_start_stop(
    char *test_lisp_file, char *expected_output) {
    int i;
    struct file_with_eof_flag streamobj;
    streamobj.file = fopen(test_lisp_file, "rb");
    streamobj.characters_to_read = 0;

    BISTACK *bs = bistack_new(1<<14);
    bistack_pushdir(bs, BS_BACKWARD);
    ENVIRONMENT *environment = environment_new(bs);
    READER *reader = reader_new(environment);
    reader_set_getc(reader, mygetc, &streamobj);
    reader_set_putc(reader, myputc, NULL);

    // zero bs
    bistack_zero(bs);
    while(!reader->is_completed) {
        char res = reader_read(reader);
        // reset streamobj to read next char
        streamobj.characters_to_read += 1;

        // find first two-zeros
        if (reader->reader_context == NULL) {
            continue;
        }
    }
    void *zeros = memmem(reader->cell, 1<<14, "\0\0", 2);
    int bytes_to_compare = zeros - (void*)reader->cell;
    printf("comparing %d bytes\n", bytes_to_compare);
    for (i=0; i<bytes_to_compare; i++) {
        if (((char*)reader->cell)[i] != expected_output[i]) {
        printf("byte:%03d expected[0x%02x] got[0x%02x]\n",
            i, ((char*)reader->cell)[i], expected_output[i]);
        mu_assert("bytes didn't match", FALSE);
        }
    }

    return 0;
}

static char *all_tests() {
    char *testdata[][2] = {
        {"tests/samples/sample1.lisp", result_of_sample1_lisp},
        {"tests/samples/sample2.lisp", result_of_sample2_lisp},
    };
    TEST_FN test_fns[] = {
        test_reader_result,
        test_reader_pprint,
        test_reader_start_stop,
    };

    char *message = NULL;
    for (int testdata_i = 0;
            testdata_i < sizeof(testdata)/sizeof(char*);
            testdata_i++) {
        for (int testfn_i = 0;
                testfn_i < sizeof(test_fns)/(sizeof(TEST_FN));
                testfn_i++) {
            message = test_fns[testfn_i](
                testdata[testdata_i][0], testdata[testdata_i][1]);
            tests_run++;
            if (message) {
                return message;
            }
        }
    }
    return 0;
}


int main(int argc, char **argv) {
  char *result = all_tests();
  if (result != 0) {
      printf("%s\n", result);
  }
  else {
      printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);

  return !!result;
}

/* results of reading smaple.lisp with some extra zeros at the end */
char result_of_sample1_lisp[] = {
  0x03, 0x01, 0x15, 0x92, 0x64, 0x65, 0x66, 0x75, 0x6e, 0x11, 0x14, 0x6d,
  0x6f, 0x76, 0x65, 0x03, 0x01, 0x05, 0x05, 0x6e, 0x11, 0xf5, 0x66, 0x72,
  0x6f, 0x6d, 0x09, 0x6c, 0x74, 0x6f, 0x0d, 0x3f, 0x76, 0x69, 0x61, 0xc3,
  0x00, 0x11, 0xb1, 0x63, 0x6f, 0x6e, 0x64, 0x83, 0x00, 0xc3, 0x00, 0x05,
  0xc5, 0x3d, 0x05, 0x05, 0x6e, 0x0e, 0x00, 0x43, 0x01, 0x19, 0xd7, 0x66,
  0x6f, 0x72, 0x6d, 0x61, 0x74, 0x05, 0x9e, 0x74, 0x5d, 0x56, 0x22, 0x4d,
  0x6f, 0x76, 0x65, 0x20, 0x66, 0x72, 0x6f, 0x6d, 0x20, 0x7e, 0x41, 0x20,
  0x74, 0x6f, 0x20, 0x7e, 0x41, 0x2e, 0x7e, 0x25, 0x22, 0x11, 0xf5, 0x66,
  0x72, 0x6f, 0x6d, 0x09, 0x6c, 0x74, 0x6f, 0x03, 0x01, 0x05, 0x9e, 0x74,
  0x43, 0x01, 0x11, 0x14, 0x6d, 0x6f, 0x76, 0x65, 0xc3, 0x00, 0x05, 0xac,
  0x2d, 0x05, 0x05, 0x6e, 0x0e, 0x00, 0x11, 0xf5, 0x66, 0x72, 0x6f, 0x6d,
  0x0d, 0x3f, 0x76, 0x69, 0x61, 0x09, 0x6c, 0x74, 0x6f, 0x43, 0x01, 0x19,
  0xd7, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x74, 0x05, 0x9e, 0x74, 0x5d, 0x56,
  0x22, 0x4d, 0x6f, 0x76, 0x65, 0x20, 0x66, 0x72, 0x6f, 0x6d, 0x20, 0x7e,
  0x41, 0x20, 0x74, 0x6f, 0x20, 0x7e, 0x41, 0x2e, 0x7e, 0x25, 0x22, 0x11,
  0xf5, 0x66, 0x72, 0x6f, 0x6d, 0x09, 0x6c, 0x74, 0x6f, 0x43, 0x01, 0x11,
  0x14, 0x6d, 0x6f, 0x76, 0x65, 0xc3, 0x00, 0x05, 0xac, 0x2d, 0x05, 0x05,
  0x6e, 0x0e, 0x00, 0x0d, 0x3f, 0x76, 0x69, 0x61, 0x09, 0x6c, 0x74, 0x6f,
  0x11, 0xf5, 0x66, 0x72, 0x6f, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

char result_of_sample2_lisp[1000];