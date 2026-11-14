#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include <x86intrin.h>

#define IS_MAIN
#include "base.c"
#include "profile.c"

create_slice_type(slice(u8));
typedef struct {
  slice(u8) key;
  u32 val;
  u32 next;
} Node;
create_slice_type(Node);

typedef union {
  Node Node_Variant;
  slice(u8) Literal_Variant;
} Node_Literal_Union;

typedef struct {
  slice(Node) buff;
  slice(slice(u8)) buff_literals;
  u32 val;
} Json_Output;

u8 read_file(char *input_filename, slice(u8) * input_data);
Json_Output parse_json(slice(u8) input_data);
u32 parse_element(slice(Node) * buff, slice(slice(u8)) * buff_literals,
                  slice(u8) input_data, u32 *i);

void print_json_element(slice(Node) buff, slice(slice(u8)) buff_literals,
                        Node node);

Node json_lookup_node(slice(Node) buff, slice(slice(u8)) buff_literals,
                      Node node, slice(u8) key);
slice(u8) json_lookup_literal(slice(Node) buff, slice(slice(u8)) buff_literals,
                              Node node, slice(u8) key);

int main(int argc, char *argv[]) {
  int error = 0;
  if (argc < 2) {
    printf_error("Require json filename and optional answer filename argument\n"
                 "Usage: %s [haversine_input.json]\n"
                 "       %s [haversine_input.json] [answer.f64]\n",
                 argv[0], argv[0]);
    error = 1;
  }

  slice(u8) input_data = {};
  if (!error) {
    char *input_filename = argv[1];
    error = read_file(input_filename, &input_data);
  }

  if (!error) {
    Json_Output json_output = parse_json(input_data);
    Node json_entry = json_output.buff.ptr[json_output.val];
    Node node = json_entry;
    print_json_element(json_output.buff, json_output.buff_literals, node);
  }

  return error;
}

u8 read_file(char *input_filename, slice(u8) * input_data) {
  profile_function;
  FILE *input_file = fopen(input_filename, "r");
  u8 error = 0;
  if (input_file == NULL) {
    printf_error("Unable to open file '%s'\n", input_filename);
    error = 1;
  } else {
    fseek(input_file, 0, SEEK_END);
    u32 len = ftell(input_file);
    rewind(input_file);
    input_data->ptr = malloc(len);
    if (input_data->ptr) {
      fread(input_data->ptr, 1, len, input_file);
      input_data->len = len;
    }
  }
  return error;
}

Json_Output parse_json(slice(u8) input_data) {
  u32 i = 0;
  slice(Node) buff = {
      .ptr = malloc(input_data.len * sizeof(Node)),
      .len = 1, // 0 means unavailable
  };
  slice(slice(u8)) buff_literals = {
      .ptr = malloc(input_data.len * sizeof(slice(u8))),
      .len = 1,
  };
  u32 val = parse_element(&buff, &buff_literals, input_data, &i);
  return (Json_Output){
      .buff = buff, .buff_literals = buff_literals, .val = val};
}

// error handling, whats that?
u32 parse_element(slice(Node) * buff, slice(slice(u8)) * buff_literals,
                  slice(u8) input_data, u32 *i) {
  u8 character = input_data.ptr[*i];
  *i += 1;
  switch (character) {
  default: {
  } break;
  case ' ':
    break;
  case '{':

  {
    u32 map_i = buff->len;
    Node *prev_node = &(Node){};
    while (true) {
      u8 character = input_data.ptr[*i];
      *i += 1;

      switch (character) {
      case ' ':
      case ',':
        break;

      case '"': {
        slice(u8) key = {.ptr = &input_data.ptr[*i]};
        u8 character = 0;
        while (true) {
          character = input_data.ptr[*i];
          *i += 1;
          if (character == '"') {
            key.len = &input_data.ptr[*i - 1] - key.ptr;
            prev_node->next = buff->len;
            Node *node = &buff->ptr[buff->len];
            buff->len += 1;
            node->key = key;
            *i += 1; // consume ':'
            node->val = parse_element(buff, buff_literals, input_data, i);
            prev_node = node;
            break;
          }
        }
      } break;
      case '}':
        goto done_map;
      }
    }
  done_map:
    return map_i;

  } break;
  case '[': {
    u32 list_i = buff->len;
    Node *prev_node = &buff->ptr[list_i];
    // Node *prev_node = &(Node){};
    while (true) {
      u8 character = input_data.ptr[*i];
      *i += 1;
      switch (character) {
      default:
        prev_node->next = buff->len;
        Node *node = &buff->ptr[buff->len];
        buff->len += 1;
        *i -= 1;
        node->val = parse_element(buff, buff_literals, input_data, i);
        prev_node = node;
        break;

      case ' ':
      case '\n':
      case ',':
        break;

      case ']':
        goto done_list;
      }
    }

  done_list:
    return list_i;
  } break;
  case '"':
    // should not hapen so not gonna handle
    break;
  case '-':
  case '0' ... '9': {
    slice(u8) number = {.ptr = &input_data.ptr[*i]};
    u8 character = 0;
    while (true) {
      character = input_data.ptr[*i];
      if (character < '0' || character > '9') {
        if (character == '.') {
          *i += 1;
          character = input_data.ptr[*i];
          while (character <= '9' && character >= '0') {
            *i += 1;
            character = input_data.ptr[*i];
          }
        }
        number.len = &input_data.ptr[*i - 1] - number.ptr;
        buff_literals->ptr[buff_literals->len] = number;
        u32 number_i =
            buff_literals->len | 0x80000000; // set bit at beginning to 1
        buff_literals->len += 1;
        return number_i;
      }
      *i += 1;
    }

  } break;
  }
  return 0;
}
void print_json_element(slice(Node) buff, slice(slice(u8)) buff_literals,
                        Node node) {

  if (node.val & 0x80000000) {
    u32 literal_i = node.val & 0x7FFFFFFF; // remove bit at beginning
    slice(u8) number = buff_literals.ptr[literal_i];
    printf("%.*s", number.len, number.ptr);
  } else {
    Node obj_node = buff.ptr[node.val];
    if (obj_node.key.ptr != 0) {
      printf("{");
      do {
        printf("%.*s: ", obj_node.key.len, obj_node.key.ptr);
        print_json_element(buff, buff_literals, buff.ptr[obj_node.val]);
        printf(",");
        obj_node = buff.ptr[obj_node.next];
      } while (obj_node.next);
      printf("}");
    } else {
      printf("[");
      do {
        print_json_element(buff, buff_literals, buff.ptr[obj_node.val]);
        printf(",");
        obj_node = buff.ptr[obj_node.next];
      } while (obj_node.next);
      printf("]");
    }
  }
}

Node json_lookup_node(slice(Node) buff, slice(slice(u8)) buff_literals,
                      Node node, slice(u8) key) {
  Node res = {};
  while (!is_equal_u8_slice(node.key, key)) {
    node = buff.ptr[node.next];
  }
  if (is_equal_u8_slice(node.key, key)) {
    res = buff.ptr[node.val];
  }
  return res;
}

slice(u8) json_lookup_literal(slice(Node) buff, slice(slice(u8)) buff_literals,
                              Node node, slice(u8) key) {
  slice(u8) res = {};
  while (!is_equal_u8_slice(node.key, key)) {
    node = buff.ptr[node.next];
  }
  if (is_equal_u8_slice(node.key, key)) {
    u32 literal_i = node.val & 0x7FFFFFFF; // remove bit at beginning
    res = buff_literals.ptr[literal_i];
  }
  return res;
}

#define static_assert(cond, message)                                           \
  typedef char concat(static_assert_, message)[(cond) ? 1 : -1]

static_assert(__COUNTER__ < MAX_PROFILE_POINTS,
              concat(can_not_have_more_than_, MAX_PROFILE_POINTS));
