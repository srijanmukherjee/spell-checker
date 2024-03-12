#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DICTIONARY_CAPACITY 1024
#define BUFFER_SIZE 4096

typedef struct {
  char *str;
  char *pos;
  char *definition;
} Word;

typedef struct {
  Word *words;
  size_t n;
  size_t capacity;
} Dictionary;

char buf[BUFFER_SIZE];

char *lowercase(char *s) {
  if (s == NULL)
    return NULL;
  char *ptr = s;
  while (*ptr != '\0') {
    *ptr = tolower(*ptr);
    ptr++;
  }
  return s;
}

/**
 * load words from dictionary
 *
 * csv line format: word,pos,definition
 */
Dictionary *load_dictionary(const char *filename) {

  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    return NULL;
  }

  Dictionary *dictionary = malloc(sizeof(Dictionary));
  if (dictionary == NULL) {
    fclose(fp);
    return NULL;
  }

  dictionary->words = malloc(sizeof(Word) * DICTIONARY_CAPACITY);
  dictionary->capacity = DICTIONARY_CAPACITY;
  dictionary->n = 0;

  while (fgets(buf, BUFFER_SIZE, fp) != NULL) {
    char *entry = strdup(buf);
    char *entry_beg = entry;

    char *word = strsep(&entry, ",");
    char *pos = strsep(&entry, ",");
    char *definition = entry != NULL ? strdup(entry) : NULL;

    if (dictionary->n == dictionary->capacity) {
      dictionary->capacity += DICTIONARY_CAPACITY;
      dictionary->words =
          realloc(dictionary->words, sizeof(Word) * dictionary->capacity);
      if (dictionary->words == NULL) {
        fclose(fp);
        free(entry_beg);
        return NULL;
      }
    }

    dictionary->words[dictionary->n].str = lowercase(strdup(word));
    dictionary->words[dictionary->n].pos = pos == NULL ? NULL : strdup(pos);
    dictionary->words[dictionary->n].definition = definition;
    dictionary->n++;

    free(entry_beg);
  }

  // deallocate unused capacity
  dictionary->words = realloc(dictionary->words, sizeof(Word) * dictionary->n);
  dictionary->capacity = dictionary->n;

  fclose(fp);

  return dictionary;
}

inline static int min(int a, int b) { return a > b ? b : a; }

int distance(char *from, char *to, int m, int n) {
  static int dp[4096][4096];

  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      int dist = INT_MAX;

      if (i == 0 && j == 0) {
        dist = 0;
      }

      if (i > 0) {
        dist = min(dist, dp[i - 1][j] + 1);
      }

      if (j > 0) {
        dist = min(dist, dp[i][j - 1] + 1);
      }

      if (i > 0 && j > 0) {
        int match = from[i] == to[j] ? 0 : 1;
        dist = min(dist, dp[i - 1][j - 1] + match);
      }

      if (i > 1 && j > 1 && from[i] == to[j - 1] && from[i - 1] == to[j]) {
        int match = from[i] == to[j] ? 0 : 1;
        dist = min(dist, dp[i - 2][j - 2] + match);
      }

      dp[i][j] = dist;
    }
  }

  return dp[m - 1][n - 1];
}

int main() {
  Dictionary *dictionary = load_dictionary("./dictionary.csv");
  if (dictionary == NULL) {
    fprintf(stderr, "error: failed to load the dictionary: %s\n",
            strerror(errno));
    exit(1);
  }

  char *input = fgets(buf, BUFFER_SIZE - 1, stdin);
  int n = strlen(input);

  // remove \n character from the end
  if (n > 1 && input[n - 1] == '\n')
    input[--n] = 0;

  input = lowercase(input);

  int nearest_word = 0;
  int nearest_distance = distance(input, dictionary->words[0].str, n,
                                  strlen(dictionary->words[0].str));

  for (int i = 1; i < dictionary->n && nearest_distance > 0; i++) {
    int dist = distance(input, dictionary->words[i].str, n,
                        strlen(dictionary->words[i].str));
    if (dist <= nearest_distance) {
      nearest_word = i;
      nearest_distance = dist;
    }
  }

  if (nearest_distance == 0) {
    printf("correct word: %s\n", dictionary->words[nearest_word].str);
  } else {
    printf("correction: %s (edit distance: %d)\n",
           dictionary->words[nearest_word].str, nearest_distance);
  }

  return 0;
}