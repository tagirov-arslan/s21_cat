#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct arguments {
  int b, n, s, t, v, e;
} arguments;

arguments arg_parser(int argc, char** argv);
void output(arguments* flag, char** argv, int argc);
void outline(arguments* flag, char* line, int n);
int v_output(char ch);

int main(int argc, char* argv[]) {
  arguments flag = arg_parser(argc, argv);
  output(&flag, argv, argc);
  return 0;
}

arguments arg_parser(int argc, char** argv) {
  arguments flag = {0};
  struct option long_option[] = {{"number", 0, NULL, 'n'},
                                 {"number-nonblank", 0, NULL, 'b'},
                                 {"squeeze-blank", 0, NULL, 's'},
                                 {0, 0, 0, 0}};
  int opt = 0;
  for (; opt != -1;
       opt = getopt_long(argc, argv, "+bnsEeTtv", long_option, 0)) {
    switch (opt) {
      case 'b':
        flag.b = 1;
        break;
      case 'n':
        flag.n = 1;
        break;
      case 's':
        flag.s = 1;
        break;
      case 'E':
        flag.e = 1;
        break;
      case 'e':
        flag.e = 1;
        flag.v = 1;
        break;
      case 'T':
        flag.t = 1;
        break;
      case 'v':
        flag.v = 1;
        break;
      case 't':
        flag.t = 1;
        flag.v = 1;
        break;
      case '?':
        perror("ERROR");
        exit(1);
        break;
      default:;
        break;
    }
  }
  if (flag.b == 1) flag.n = 0;
  return flag;
}

void output(arguments* flag, char** argv, int argc) {
  for (int i = optind; i < argc; i++) {
    FILE* f = fopen(argv[i], "r");
    if (f != NULL) {
      char* line = NULL;
      size_t memline = 0;
      int symbol = 0;

      int line_count = 1;
      int empty_c = 0;
      symbol = getline(&line, &memline, f);

      while (symbol != -1) {
        if (line[0] == '\n') {
          empty_c++;
        } else {
          empty_c = 0;
        }
        if (flag->s && empty_c > 1) {
          symbol = getline(&line, &memline, f);
        } else {
          if (flag->b && line[0] != '\n') {
            printf("%6d\t", line_count);
            line_count++;
          }

          if (flag->n) {
            printf("%6d\t", line_count);
            line_count++;
          }

          outline(flag, line, symbol);
          symbol = getline(&line, &memline, f);
        }
      }
      if (line) free(line);
    }
    if (f) fclose(f);
  }
}

void outline(arguments* flag, char* line, int n) {
  for (int i = 0; i < n; i++) {
    if (flag->e && line[i] == '\n') {
      if (flag->e && (flag->b) == 1 && line[0] == '\n') {
        printf("      \t$");
      } else {
        putchar('$');
      }
    }
    if (flag->t && line[i] == '\t') {
      printf("^I");
      continue;
    }

    if (flag->v) {
      line[i] = v_output(line[i]);
    }
    putchar(line[i]);
  }
}

int v_output(char ch) {
  if (ch == '\n' || ch == '\t') return ch;

  if (ch >= 0 && ch < 32) {
    putchar('^');
    ch = ch + 64;
  } else if (ch == 127) {
    putchar('^');
    ch = '?';
  }

  return ch;
}
