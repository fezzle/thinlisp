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

char* memory_check(char *got, char *expected) {
    // Compares arrays a and b until two zeros are found or 64k, 
    // returns TRUE if same
    static char message[1024];
    int i;
    void *got_zeros = memmem(got, 1<<16, "\0\0", 2);
    int got_bytes = got_zeros - (void*)got;
    void *expected_zeros = memmem(expected, 1<<16, "\0\0", 2);
    int expected_bytes = expected_zeros - (void*)expected;
    // use the higher number of bytes - the compare will fail when the smaller
    //  one's zeros are found
    int bytes_to_compare = (
      got_bytes < expected_bytes ? expected_bytes : got_bytes);
    for (i=0; i<bytes_to_compare; i++) {
        if (got[i] != expected[i]) {
            snprintf(
                message, 
                sizeof(message)-1, 
                "byte:%03d expected[0x%02x] got[0x%02x]\n",
                i, 
                expected[i], 
                got[i]);
            message[sizeof(message)-1] = '\0';
            return message;
        }
    }
    return NULL;
}
    
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

  BISTACK *bs = bistack_new(1<<18);
  bistack_pushdir(bs, BS_BACKWARD);
  ENVIRONMENT *environment = environment_new(bs);
  READER *reader = reader_new(environment);
  reader_set_getc(reader, mygetc, &streamobj);
  while (!feof(streamobj.file)) {
    bool res = reader_read(reader);
    mu_assert("reader should complete", res);
  }
  mu_assert("root cell type should be list",
    reader->reader_context->cellheader->List.type == AST_LIST);
  char *memory_check_res = memory_check(
        (char*)reader->reader_context->cellheader, expected_output);
  mu_assert(memory_check_res, memory_check_res == NULL);
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

  BISTACK *bs = bistack_new(1<<18);
  bistack_pushdir(bs, BS_BACKWARD);
  ENVIRONMENT *environment = environment_new(bs);
  READER *reader = reader_new(environment);
  reader_set_getc(reader, mygetc, &streamobj);
  reader_set_putc(reader, myputc, NULL);
  while (!feof(streamobj.file)) {
    bool res = reader_read(reader);
    mu_assert("reader should complete", res);
  }
  reader_pprint(reader);
  mu_assert(
    "root cell type should be list",
    reader->reader_context->cellheader->List.type == AST_LIST);
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

    BISTACK *bs = bistack_new(1<<18);
    bistack_pushdir(bs, BS_BACKWARD);
    ENVIRONMENT *environment = environment_new(bs);
    READER *reader = reader_new(environment);
    reader_set_getc(reader, mygetc, &streamobj);
    reader_set_putc(reader, myputc, NULL);

    // zero bs
    bistack_zero(bs);
    while(!feof(streamobj.file)) {
        char res = reader_read(reader);
        // reset streamobj to read next char
        streamobj.characters_to_read += 1;
    }
    char *memory_check_res = memory_check(
        (char*)reader->reader_context->cellheader, expected_output);
    mu_assert(memory_check_res, memory_check_res == NULL);
    return 0;
}

static char *all_tests() {
    static char fullmessage[1024];
    struct TESTDATA {
        char *test_lisp_file;
        char *expected_result;
    } testdata[] = {
        {
            .test_lisp_file="tests/samples/sample1.lisp", 
            .expected_result=result_of_sample1_lisp
        },
        {
            .test_lisp_file="tests/samples/sample2.lisp", 
            .expected_result=result_of_sample2_lisp
        },
    };

    struct TESTFUNCTIONS {
        TEST_FN testfn;
        char *fnname;
    } testfns[] = {
        {
            .testfn=test_reader_result, 
            .fnname="test_reader_result"
        },
        {
            .testfn=test_reader_pprint, 
            .fnname="test_reader_pprint"
        },
        {
            .testfn=test_reader_start_stop, 
            .fnname="test_reader_start_stop"
        },
    };

    char *message = NULL;
    for (int testdata_i = 0;
            testdata_i < sizeof(testdata)/sizeof(struct TESTDATA);
            testdata_i++) {
        for (int testfn_i = 0;
                testfn_i < sizeof(testfns)/sizeof(struct TESTFUNCTIONS);
                testfn_i++) {
            printf("Running %s: ", testfns[testfn_i].fnname);
            message = testfns[testfn_i].testfn(
                testdata[testdata_i].test_lisp_file, 
                testdata[testdata_i].expected_result);
            tests_run++;
            if (message) {
                printf("FAILED\n");
                snprintf(fullmessage, 
                  sizeof(fullmessage)-1,
                  "%s-%s: %s", 
                  testdata[testdata_i].test_lisp_file,
                  testfns[testfn_i].fnname,
                  message);
                fullmessage[sizeof(fullmessage)-1] = '\0';
                return fullmessage;
            } else {
                printf("PASSED\n");
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
  0x43, 0x00, // reader->reader_context headers are now always list
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

char result_of_sample2_lisp[1000] = {'\0', '\0'};