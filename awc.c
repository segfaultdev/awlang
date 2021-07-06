#include <awlang.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char *argv[]) {
  const char *path = NULL;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-o")) path = argv[++i];
  }

  FILE *file = fopen(path, "w");
  if (!file) return 1;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-o")) {
      i++;
    } else {
      FILE *input = fopen(argv[i], "r");
      if (!input) return 1;

      fseek(input, 0, SEEK_END);
      size_t size = ftell(input);
      fseek(input, 0, SEEK_SET);

      char *code = calloc(size + 1, 1);
      char *temp = code;

      fread(code, 1, size, input);
      fclose(input);

      aw_parse(file, (const char **)(&temp));
      free(code);
    }
  }

  fclose(file);

  return 0;
}
