#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum ap_type {
  AP_FLAG,
  AP_FVALUE
};

struct ap_value {
  // information provided by user
  enum ap_type type;
  const char *desc;
  const char *sname; // short name
  const char *lname; // long name

  // information generated after parsing
  char is_exists; // if the flag exists
  const char* value; // value when ap_type is AP_FVALUE
};

struct ap_value *ap_value_init(enum ap_type type, 
                               const char *desc,
                               const char *sname,
                               const char *lname);
int ap_value_delete(struct ap_value *value);

struct ap_parser {
  const char *program_name;
  const char *desc;
  int cap;
  int len;
  struct ap_value **values;
};

struct ap_parser *ap_parser_init(const char *program_name, const char *desc);
int ap_parser_delete(struct ap_parser *parser);
int ap_parser_add_argument(struct ap_parser *parser, struct ap_value* value);
int ap_parser_parse(struct ap_parser *parser, int argc, const char **argv);
int ap_parser_usage(struct ap_parser *parser);

// definitions
struct ap_value *ap_value_init(enum ap_type type, 
                               const char *desc,
                               const char *sname,
                               const char *lname) {
  struct ap_value *value = (struct ap_value*)malloc(sizeof(struct ap_value));
  if (value == NULL) {
    return NULL;
  }
  value->type = type;
  value->desc = desc;
  value->sname = sname;
  value->lname = lname;
  value->is_exists = 0;
  value->value = NULL;
  return value;
}

int ap_value_delete(struct ap_value *value) {
  if (value == NULL) {
    return -1;
  }
  free(value);
  return 0;
}

struct ap_parser *ap_parser_init(const char* program_name, const char* desc) {
  struct ap_parser *parser = (struct ap_parser *)malloc(sizeof(struct ap_parser));
  if (parser == NULL) {
    return NULL;
  }
  parser->program_name = program_name;
  parser->desc = desc;
  parser->cap = 0;
  parser->len = 0;
  parser->values = NULL;
  return parser;
}

int ap_parser_delete(struct ap_parser *parser) {
  if (parser == NULL) {
    return -1;
  }
  free(parser->values);
  free(parser);
  return 0;
}

int ap_parser_add_argument(struct ap_parser *parser, struct ap_value *value) {
  if (parser == NULL || value == NULL) {
    return -1;
  }

  if (parser->len >= parser->cap) {
    parser->cap += 8;
    struct ap_value **temp_values = malloc(parser->cap * sizeof(struct ap_value));
    if (temp_values == NULL) {
      return -1;
    }
    for (int i = 0; i < parser->len; i++) {
      temp_values[i] = parser->values[i];
    }
    free(parser->values);
    parser->values = temp_values;
  }
  parser->values[parser->len] = value;
  parser->len++;

  return 0;
}

int ap_parser_parse(struct ap_parser *parser, int argc, const char **argv) {
  if (parser == NULL) {
    return -1;
  }

  int i = 1;
  while (i < argc) {
    char found = 0;
    for (int j = 0; j < parser->len; j++) {
      struct ap_value *value = parser->values[j];
      if (value->sname != NULL && strcmp(argv[i], value->sname) == 0 || 
          value->lname != NULL && strcmp(argv[i], value->lname) == 0) {
        value->is_exists = 1;
        if (value->type == AP_FLAG) {
          i++;
        } else {
          if (i + 1 >= argc) {
            return -1;
          }
          value->value = argv[i + 1];
          i += 2;
        }
        found = 1;
        break;
      }
    }
    if (!found) {
      break;
    }
  }
  if (i == argc) {
    return 0;
  }
  return -1;
}

int ap_parser_help(struct ap_parser *parser) {
  printf("Description:\n  %s\n\n", parser->desc);
  printf("Usage:\n  %s [options]\n\n", parser->program_name);
  printf("Options:\n");
  for (int i = 0; i < parser->len; i++) {
    struct ap_value *value = parser->values[i];
    char line[120] = "";
    strcat(line, "  ");
    if (value->sname != NULL) {
      strcat(line, value->sname);
      if (value->type == AP_FVALUE) {
        strcat(line, " <value>");
      }
    }
    if (value->lname != NULL) {
      if (value->sname != NULL) {
        strcat(line, ", ");
      }
      strcat(line, value->lname);
      if (value->type == AP_FVALUE) {
        strcat(line, " <value>");
      }
    }
    printf("%-40s  ; %s\n", line, value->desc);
  }
  return 0;
}

#endif // ARGPARSER_H
