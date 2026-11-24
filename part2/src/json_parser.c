#ifndef IS_MAIN
#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdio.h>
#include <x86intrin.h>
#define IS_MAIN
#include "base.c"
#include "profile.c"
#endif

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

Json_Output parse_json(slice(u8) input_data);
u32 parse_element(slice(Node) * buff, slice(slice(u8)) * buff_literals,
                  slice(u8) input_data, u32 *i);

void print_json_element(slice(Node) buff, slice(slice(u8)) buff_literals,
                        u32 node_i);

Node json_lookup_node(slice(Node) buff, slice(slice(u8)) buff_literals,
                      Node node, slice(u8) key);
slice(u8) json_lookup_literal(slice(Node) buff, slice(slice(u8)) buff_literals,
                              Node node, slice(u8) key);

f64 parse_f64(slice(u8) number) {
  u32 i = 0;
  bool pos = true;

  if (number.ptr[i] == '-') {
    pos = false;
    i += 1;
  }
  i64 integer = 0;
  while (i < number.len && number.ptr[i] >= '0' && number.ptr[i] <= '9') {
    integer = integer * 10 + number.ptr[i] - '0';
    i += 1;
  }
  u64 fraction = 0;
  u16 decimal_point_pos = 0;
  if (i < number.len && number.ptr[i] == '.') {
    i += 1;
    while (i < number.len && number.ptr[i] >= '0' && number.ptr[i] <= '9') {
      fraction = fraction * 10 + number.ptr[i] - '0';
      decimal_point_pos += 1;
      i += 1;
    }
  }

  f64 pos_val = (f64)integer + (f64)fraction * pow(10, -decimal_point_pos);
  return pos ? pos_val : pos_val * -1;
}

void parse_haversine_data(Json_Output json_output,
                          HaversineDataVector *haversine_data_vector) {
  slice(Node) buff = json_output.buff;
  slice(slice(u8)) buff_literals = json_output.buff_literals;
  u32 node_i = json_output.val;
  if (!(node_i & 0x80000000)) {
    Node node = buff.ptr[node_i];
    Node list_node =
        json_lookup_node(buff, buff_literals, node, make_string("pairs"));
    while (true) {
      Node pairs_node = buff.ptr[list_node.val];
      slice(u8) x0 = json_lookup_literal(buff, buff_literals, pairs_node,
                                         make_string("x0"));
      slice(u8) y0 = json_lookup_literal(buff, buff_literals, pairs_node,
                                         make_string("y0"));
      slice(u8) x1 = json_lookup_literal(buff, buff_literals, pairs_node,
                                         make_string("x1"));
      slice(u8) y1 = json_lookup_literal(buff, buff_literals, pairs_node,
                                         make_string("y1"));
      HaversineData haversine_pair = {
          .x0 = parse_f64(x0),
          .y0 = parse_f64(y0),
          .x1 = parse_f64(x1),
          .y1 = parse_f64(y1),
      };
      haversine_data_vector->ptr[haversine_data_vector->len] = haversine_pair;
      haversine_data_vector->len += 1;
      if (!list_node.next)
        break;
      list_node = buff.ptr[list_node.next];
    }
  } else {
    // return 1;
  }
}

Json_Output parse_json(slice(u8) input_data) {
  u32 i = 0;
  // profile_function;
  slice(Node) buff = {
      .ptr = calloc(input_data.len, sizeof(Node)),
      .len = 1, // 0 means unavailable
  };
  slice(slice(u8)) buff_literals = {
      .ptr = calloc(input_data.len, sizeof(slice(u8))),
      .len = 1,
  };
  u32 val = parse_element(&buff, &buff_literals, input_data, &i);
  return (Json_Output){
      .buff = buff, .buff_literals = buff_literals, .val = val};
}

// error handling, whats that?
u32 parse_element(slice(Node) * buff, slice(slice(u8)) * buff_literals,
                  slice(u8) input_data, u32 *i) {
  profile_function;
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
        // printf("%s\n", &input_data.ptr[*i]);
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
    Node *prev_node = &(Node){};
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
    slice(u8) number = {.ptr = &input_data.ptr[*i - 1]};
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
                        u32 node_i) {
  if (node_i & 0x80000000) {
    u32 literal_i = node_i & 0x7FFFFFFF; // remove bit at beginning
    slice(u8) number = buff_literals.ptr[literal_i];
    printf("%.*s", number.len, number.ptr);
  } else {
    Node node = buff.ptr[node_i];
    if (node.key.ptr != 0) {
      printf("{");
      while (true) {
        printf("\"%.*s\": ", node.key.len, node.key.ptr);
        print_json_element(buff, buff_literals, node.val);
        if (!node.next)
          break;
        node = buff.ptr[node.next];
        printf(", ");
      }
      printf("}");
    } else {
      printf("[");
      while (true) {
        print_json_element(buff, buff_literals, node.val);
        if (!node.next)
          break;
        node = buff.ptr[node.next];
        printf(",");
      }
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
