#include "log.h"
#include "utarray.h"
#include "utstring.h"
#include "unity.h"
#include "s7.h"

/* we need both APIs for test validation */
#include "sealark.h"
#include "sunlark.h"

#include "test_vectors.h"

UT_string *buf;
UT_string *test_s;
UT_array  *result;

/* char *build_file = "test/unit/sunlark/BUILD.vectors"; */

s7_scheme *s7;

/* struct parse_state_s *parse_state; */

s7_pointer ast;
struct node_s *root;

s7_pointer is_eq_s7;
s7_pointer is_equal_s7;
s7_pointer length_s7;

void setUp(void) {
    s7 = sunlark_init();
    length_s7 = s7_name_to_value(s7, "length");

    /* is_eq_s7 = s7_name_to_value(s7, "eq?"); */
    /* is_equal_s7 = s7_name_to_value(s7, "equal?"); */
    /* ast = sunlark_parse_build_file(s7, */
    /*                                s7_list(s7, 1, */
    /*                                        s7_make_string(s7, build_file))); */
    /* root = s7_c_object_value(ast); */
}

void tearDown(void) {
    s7_quit(s7);
}

void test_vector_properties(void) {
    s7_pointer vec = sunlark_parse_string(s7, s7_make_string(s7, "[1, 2, 3]\n"));

    s7_pointer is_sunlark_node = s7_name_to_value(s7, "sunlark-node?");
    s7_pointer pred = s7_apply_function(s7, is_sunlark_node,
                                        s7_list(s7, 1, vec));
    TEST_ASSERT( pred == s7_t(s7) );

    /* length is 3 */
    s7_pointer count = s7_apply_function(s7, length_s7,
                                         s7_list(s7, 1, vec));
    TEST_ASSERT( 3 == s7_integer(count) );

    /* first item */
    s7_pointer path = s7_eval_c_string(s7, "'(0)");
    s7_pointer item = s7_apply_function(s7, vec, path);
    TEST_ASSERT( s7_is_c_object(item) );
    TEST_ASSERT( sunlark_node_tid(s7, item) == TK_INT );

    pred = s7_f(s7);
    pred = s7_apply_function(s7, is_sunlark_node, s7_list(s7, 1, item));
    TEST_ASSERT( pred == s7_t(s7) );
    pred = s7_f(s7);
    pred = s7_apply_function(s7, item,
                             s7_list(s7, 1, s7_make_symbol(s7, ":int?")));
    TEST_ASSERT( pred == s7_t(s7) );

    /* (item :$) => s7 (Scheme) int value */
    s7_pointer ival = s7_apply_function(s7, item,
                                        s7_eval_c_string(s7, "'(:$)"));
    log_debug("ival t: %s",
              s7_object_to_c_string(s7, s7_type_of(s7, ival)));

    pred = s7_f(s7);
    pred = s7_apply_function(s7, s7_name_to_value(s7, "integer?"),
                             s7_list(s7, 1, ival));
    TEST_ASSERT( pred == s7_t(s7) );
    TEST_ASSERT( s7_is_integer(ival) );
    TEST_ASSERT( s7_is_c_object(item) );
    TEST_ASSERT_EQUAL_INT( 1, s7_integer(ival) );

    /* just to be sure, check underlying c object */
    struct node_s *item_node = s7_c_object_value(item);
    TEST_ASSERT( item_node->tid == TK_INT );
    TEST_ASSERT( strncmp(item_node->s, "1", 1) == 0 );
    TEST_ASSERT( strlen(item_node->s) == 1 );

    /* to get the number out of the TK_INT node... */
    /* (node :$) => typed value: int for ints, string for strings, symbol for identifiers */

    /* (eq? item 1) => #f, but (eq? (item :val) 1) => #t */
    s7_pointer eq_1 = s7_apply_function(s7,
                                        s7_eval_c_string(s7, "eq?"),
                                        s7_list(s7, 2,
                                                ival, /* from above, (item :$) */
                                                s7_make_integer(s7, 1)));
    TEST_ASSERT( eq_1 == s7_t(s7) );
}

void test_int_vector(void) {
    s7_pointer vec = sunlark_parse_string(s7, s7_make_string(s7, "[1, 2, 3]\n"));

    s7_pointer path = s7_eval_c_string(s7, "'(0)");
    s7_pointer item = s7_apply_function(s7, vec, path);

    log_debug("item:\n%s", s7_object_to_c_string(s7, item));

    /* check type, tid */
    TEST_ASSERT( s7_is_c_object(item) );
    TEST_ASSERT( sunlark_node_tid(s7, item) == TK_INT );

    /* :$ returns typed value (all values stored as strings in AST) */
    s7_pointer ival = s7_apply_function(s7, item, s7_eval_c_string(s7, "'(:$)"));
    log_debug("ival: %s", s7_object_to_c_string(s7, ival));
    TEST_ASSERT( s7_is_integer(ival) );
    TEST_ASSERT_EQUAL_INT( 1, s7_integer(ival) );

    struct node_s *item_node = s7_c_object_value(item);
    TEST_ASSERT( item_node->tid == TK_INT );
    TEST_ASSERT( strncmp(item_node->s, "1", 1) == 0 );
    TEST_ASSERT( strlen(item_node->s) == 1 );

    /* to get the number out of the TK_INT node... */
    /* (node :$) => typed value: int for ints, string for strings, symbol for identifiers */

    /* (eq? item 1) => #f, but (eq? (item :val) 1) => #t */
    s7_pointer eq_1 = s7_apply_function(s7,
                                        s7_eval_c_string(s7, "eq?"),
                                        s7_list(s7, 2,
                                                ival, /* from above, (item :$) */
                                                s7_make_integer(s7, 1)));
    TEST_ASSERT( eq_1 == s7_t(s7) );
}

/* labels are syntactically same strings, but just in case we add
   label-aware ops */
void test_label_vector(void) {
    s7_pointer path = s7_eval_c_string(s7,
                       "'(:targets 0 :bindings string_vec :value)");
    s7_pointer bvalue = s7_apply_function(s7, ast, path);

    log_debug("bvalue:\n%s", s7_object_to_c_string(s7, bvalue));

    /* check type, tid */
    TEST_ASSERT( s7_is_c_object(bvalue) );
    TEST_ASSERT( sunlark_node_tid(s7, bvalue) == TK_List_Expr );

    struct node_s *bvalue_node = s7_c_object_value(bvalue);
    TEST_ASSERT( bvalue_node->tid == TK_List_Expr );
}

void test_string_vector(void) {
    s7_pointer vec = sunlark_parse_string(s7, s7_make_string(s7, "[\"a\", \"b\", \"c\"]\n"));

    s7_pointer path = s7_eval_c_string(s7, "'(0)");
    s7_pointer item = s7_apply_function(s7, vec, path);

    /* check type, tid */
    TEST_ASSERT( s7_is_c_object(item) );
    TEST_ASSERT( sunlark_node_tid(s7, item) == TK_STRING );

    /* :$ returns typed value (all values stored as strings in AST) */
    s7_pointer str_val = s7_apply_function(s7, item, s7_eval_c_string(s7, "'(:$)"));
    TEST_ASSERT( s7_is_string(str_val) );
    const char *a = s7_string(str_val);
    TEST_ASSERT( 1 == strlen(a) );
    TEST_ASSERT_EQUAL_STRING( "a", a );

    struct node_s *item_node = s7_c_object_value(item);
    TEST_ASSERT( item_node->tid == TK_STRING );
    TEST_ASSERT( strncmp(item_node->s, "a", 1) == 0 );
    TEST_ASSERT( strlen(item_node->s) == 1 );

    /* (eq? item 1) => #f, but (eq? (item :val) 1) => #t */
    s7_pointer is_eq = s7_apply_function(s7,
                                        s7_eval_c_string(s7, "eq?"),
                                        s7_list(s7, 2,
                                                str_val, /* from above, (item :$) */
                                                s7_make_string(s7, "a")));
    TEST_ASSERT( is_eq == s7_f(s7) );
    is_eq = s7_f(s7);
    is_eq = s7_apply_function(s7,
                                        s7_eval_c_string(s7, "equal?"),
                                        s7_list(s7, 2,
                                                str_val, /* from above, (item :$) */
                                                s7_make_string(s7, "a")));
    TEST_ASSERT( is_eq == s7_t(s7) );
}

void test_symbol_vector(void) {
    s7_pointer vec = sunlark_parse_string(s7, s7_make_string(s7, "[avar, bvar, cvar]\n"));

    s7_pointer path = s7_eval_c_string(s7, "'(0)");
    s7_pointer item = s7_apply_function(s7, vec, path);

    /* check type, tid */
    TEST_ASSERT( s7_is_c_object(item) );
    TEST_ASSERT( sunlark_node_tid(s7, item) == TK_ID );

    s7_pointer sym_val = s7_apply_function(s7, item, s7_eval_c_string(s7, "'(:$)"));
    TEST_ASSERT( s7_is_symbol(sym_val) );
    TEST_ASSERT_EQUAL_STRING( "avar", s7_symbol_name(sym_val));

    struct node_s *item_node = s7_c_object_value(item);
    TEST_ASSERT( item_node->tid == TK_ID );
    TEST_ASSERT_EQUAL_STRING( "avar", item_node->s );
    TEST_ASSERT( strlen(item_node->s) == 4 );

    /* same syms are eq? */
    s7_pointer is_eq = s7_apply_function(s7, s7_eval_c_string(s7, "eq?"),
                                         s7_list(s7, 2,
                                                 sym_val, /* from above, (item :$) */
                                                 s7_make_symbol(s7, "avar")));
    TEST_ASSERT( is_eq == s7_t(s7) );
}

void test_set_vector(void) {
    s7_pointer vec = sunlark_parse_string(s7, s7_make_string(s7, "[avar, bvar, cvar]\n"));

    s7_pointer path = s7_eval_c_string(s7, "'(0)");
    s7_pointer item = s7_apply_function(s7, vec, path);

    /* check type, tid */
    TEST_ASSERT( s7_is_c_object(item) );
    TEST_ASSERT( sunlark_node_tid(s7, item) == TK_ID );

    s7_pointer sym_val = s7_apply_function(s7, item, s7_eval_c_string(s7, "'(:$)"));
    TEST_ASSERT( s7_is_symbol(sym_val) );
    TEST_ASSERT_EQUAL_STRING( "avar", s7_symbol_name(sym_val));

    struct node_s *item_node = s7_c_object_value(item);
    TEST_ASSERT( item_node->tid == TK_ID );
    TEST_ASSERT_EQUAL_STRING( "avar", item_node->s );
    TEST_ASSERT( strlen(item_node->s) == 4 );

    /* same syms are eq? */
    s7_pointer is_eq = s7_apply_function(s7, s7_eval_c_string(s7, "eq?"),
                                         s7_list(s7, 2,
                                                 sym_val, /* from above, (item :$) */
                                                 s7_make_symbol(s7, "avar")));
    TEST_ASSERT( is_eq == s7_t(s7) );
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_vector_properties);
    /* RUN_TEST(test_int_vector); */
    /* RUN_TEST(test_string_vector); */
    /* RUN_TEST(test_symbol_vector); */

    /* RUN_TEST(test_set_vector); */
    return UNITY_END();
}
