#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "utarray.h"

#include "log.h"
#include "s7.h"

#include "ast_nodelist_s7.h"

/* exported s7 c-types */
s7_int ast_nodelist_t = 0;

static s7_pointer g_ast_nodelist_methods_let;

/* forward decls */
/* section: identity */
static s7_pointer g_is_ast_nodelist(s7_scheme *s7, s7_pointer args);

/* section: equality */
static bool _ast_nodelists_are_value_equal(struct node_s *val1,
                                      struct node_s *val2);
static s7_pointer g_ast_nodelists_are_equal(s7_scheme *s7, s7_pointer args);
static s7_pointer g_ast_nodelists_are_equivalent(s7_scheme *s7, s7_pointer args);

/* section: getters and setters */
static s7_pointer g_ast_nodelist_ref_specialized(s7_scheme *s7, s7_pointer args);
static s7_pointer g_ast_nodelist_set_specialized(s7_scheme *s7, s7_pointer args);

static s7_pointer g_ast_nodelist_object_applicator(s7_scheme *s7, s7_pointer args);

static void _register_get_and_set(s7_scheme *s7);

/* section: display */
/* char *g_ast_nodelist_display(s7_scheme *s7, void *value); */
/* char *g_ast_nodelist_display_readably(s7_scheme *s7, void *value); */

/* section: serialization */
#define INC_PTR (workptr = workbuf + strlen(workbuf))
static s7_pointer g_ast_nodelist_to_string(s7_scheme *s7, s7_pointer args);

/* section: c-object construction */
static s7_pointer g_ast_nodelist_copy(s7_scheme *s7, s7_pointer args);
static s7_pointer g_new_ast_nodelist(s7_scheme *s7, s7_pointer args);
enum formals_e {
    FLD_C = 0, FLD_BYTE,
    FLD_STR,
    FLD_B, FLD_PB,
    FLD_SH_RT, FLD_PSH_RT,
    FLD_I, FLD_PI,
    FLD_L, FLD_PL, FLD_LL, FLD_PLL,
    FLD_F, FLD_PF, FLD_D, FLD_PD
};

/* section: c-object destruction */
static s7_pointer g_destroy_ast_nodelist(s7_scheme *s7, s7_pointer obj);

/* section: extension methods */
static void _register_c_type_methods(s7_scheme *s7, s7_int ast_nodelist_t);
static void _register_c_object_methods(s7_scheme *s7, s7_pointer ast_nodelist);

/* section: c-type configuration */
static void   _register_ast_nodelist_fns(s7_scheme *s7);

/* this is the public API that clients call: */
int configure_s7_ast_nodelist_type(s7_scheme *s7); /* public */

/* section: gc */
static s7_pointer g_ast_nodelist_gc_mark(s7_scheme *s7, s7_pointer p);

/* section: debugging */
void debug_print_s7(s7_scheme *s7, char *label, s7_pointer obj);

/* **************************************************************** */

/* **************************************************************** */
/* section: identity */
#define g_is_ast_nodelist_help "(ast_nodelist? obj) returns #t if obj is a ast_nodelist."
#define g_is_ast_nodelist_sig s7_make_signature(s7, 2, s7_make_symbol(s7, "boolean?"), s7_t(s7))
static s7_pointer g_is_ast_nodelist(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_is_ast_nodelist");
#endif
    return(s7_make_boolean(s7,
                           s7_c_object_type(s7_car(args)) == ast_nodelist_t));
}
/* /section: identity */

/* **************************************************************** */
/* section: equality

   eqv? "returns #t if obj1 and obj2 are normally regarded as the same
   object." (r7rs)

   eq? "On symbols, booleans, the empty list, pairs, and records, and
also on non-empty strings, vectors, and bytevectors, eq? and eqv? are
guaranteed to have the same behavior. On procedures, eq? must return
true if the arguments’ location tags are equal. On numbers and
characters, eq?’s behavior is implementation-dependent, but it will
always return either true or false. On empty strings, empty vectors,
and empty bytevectors, eq? may also behave differently from eqv?."
(r7rs)

   equal? "The equal? procedure, when applied to pairs, vectors,
strings and bytevectors, recursively compares them, returning #t when
the unfoldings of its arguments into (possibly infinite) trees are
equal (in the sense of equal?) as ordered trees, and #f otherwise. It
returns the same as eqv? when applied to booleans, symbols, numbers,
characters, ports, procedures, and the empty list. If two objects are
eqv?, they must be equal? as well. In all other cases, equal? may
return either #t or #f." (r7rs)

   equivalent? s7 only? same as Scheme 'equal?' ?

 */

/** _ast_nodelists_are_value_equal

    true if (possibly distinct) ast_nodelists are value-equal
 */
static bool _ast_nodelistlists_are_value_equal(UT_array *nl1,
                                               UT_array *nl2)
{
#ifdef DEBUG_TRACE
    log_debug("_ast_nodelists_are_value_equal");
#endif
    if (nl1 == nl2) return true;

    int len1 = utarray_len(nl1);
    int len2 = utarray_len(nl2);
    if (len1 != len2) return false;

    struct node_s *n1=NULL, *n2=NULL;
    int i = 1;
    while( (n1=(struct node_s*)utarray_next(nl1, n1))
           && (n2=(struct node_s*)utarray_next(nl2, n2)) ) {
        if ( !_ast_nodelists_are_value_equal(n1, n2) )
            return false;
        i++;
    }

    return(true);
}

static bool _ast_nodelists_are_value_equal(struct node_s *val1,
                                      struct node_s *val2)
{
#ifdef DEBUG_TRACE
    log_debug("_ast_nodelists_are_value_equal");
#endif
    if (val1 == val2) return true;

    if (val1->type != val2->type) return false;
    if (val1->line != val2->line) return false;
    if (val1->col  != val2->col) return false;
    if (val1->trailing_newline  != val2->trailing_newline) return false;
    if (val1->qtype  != val2->qtype) return false;
    if (val1->s != val2->s) {
        if (strncmp(val1->s, val2->s, strlen(val1->s)) != 0)
            return false;
    }

    if (val1->comments == NULL) {
        if (val2->comments != NULL) {
            return false;
        /* } else { */
        }
    } else {
        if (val2->comments == NULL)
            return false;
    }
    if ( ! _ast_nodelistlists_are_value_equal(val1->comments, val2->comments) )
        return false;

    if (val1->subnodes == NULL) {
        if (val2->subnodes != NULL) {
            return false;
        /* } else { */
        }
    } else {
        if (val2->subnodes == NULL)
            return false;
    }
    if ( ! _ast_nodelistlists_are_value_equal(val1->subnodes, val2->subnodes) )
        return false;

    return(true);
}

/** _ast_nodelists_are_c_eql

    c implementation of Scheme 'eqv?' ?

    numbers: both exact and numerically equal (=)
    pairs, vectors, bytevectors, records, strings: same mem address
 */

/** g_ast_nodelists_are_equal

    callback for Scheme 'eqv?' ?
    wrapper on _ast_nodelists_are_c_eql
 */
static s7_pointer g_ast_nodelists_are_equal(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelists_are_equal");
#endif
    return(s7_make_boolean(s7,
          /* _ast_nodelists_are_c_eql((void *)s7_c_object_value(s7_car(args)), */
          /*                      (void *)s7_c_object_value(s7_cadr(args))) */
          _ast_nodelists_are_value_equal((void *)s7_c_object_value(s7_car(args)),
                                    (void *)s7_c_object_value(s7_cadr(args)))
                           )
           );
}

/** g_ast_nodelists_are_equivalent

    implementation for Scheme 'equal?' 'eq?' ?
    true if same values
 */
#define G_AST_NODELISTS_ARE_EQUIVALENT_HELP "(equivalent? ast_nodelist1 ast_nodelist2)"
static s7_pointer g_ast_nodelists_are_equivalent(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelists_are_equivalent");
#endif

    s7_pointer arg1, arg2;

    arg1 = s7_car(args);
    arg2 = s7_cadr(args);
    if (!s7_is_c_object(arg2))
        return(s7_f(s7));

    if (s7_is_let(arg1))             /* (ast_nodelist-let (ast_nodelist)) */
        return(s7_make_boolean(s7, false));    /* checked == above */

    /* same type? type of arg1 known to be ast_nodelist */
    if (s7_c_object_type(arg2) != ast_nodelist_t)
        return(s7_make_boolean(s7, false));

    if (arg1 == arg2) { /* same c-object? */
        return(s7_t(s7));
    }

    /* compare c objects for equivalence */
    struct node_s *g1, *g2;
    g1 = (struct node_s *)s7_c_object_value(arg1);
    g2 = (struct node_s *)s7_c_object_value(arg2);

    if (g1 == g2) /* same ast_nodelist */
        return s7_t(s7);

    bool eq = _ast_nodelists_are_value_equal(g1, g2);
    if (eq)
        return(s7_t(s7));
        /* return(s7_make_boolean(s7, true)); */
    else
        return(s7_f(s7));
        /* return(s7_make_boolean(s7, false)); */
}
/* /section: equality */

/* **************************************************************** */
/* section: getters and setters */
/* get and set are special, since there are two ways to do each:
   * 1. generic get and set! (c-type methods)
   * 2. specialized ast_nodelist-get and ast_nodelist-set! (Scheme procedures)
 */

/* **************** */
/* helper fn */
static s7_pointer _ast_nodelist_lookup(s7_scheme *s7,
                                          UT_array *ast_nodelist,
                                          s7_pointer idx)
{
#ifdef DEBUG_TRACE
    log_debug("ast_nodelist_lookup");
#endif

    if ( s7_integer(idx) < nodelist_len(ast_nodelist)) {
        //FIXME implement
    } else {
        return(s7_out_of_range_error(s7,
                                     "nodelist-ref",
                                     1, idx,
                                     "less than nodelist length"));
    }
}

/* **************** */
/** g_ast_nodelist_ref_specialized

    (ast_nodelist-ref obj key)
    takes two args, a ast_nodelist object and a keyword to look up in the object.
 */
#define G_AST_NODELIST_REF_SPECIALIZED_HELP "(ast_nodelist-ref b i) returns the ast_nodelist value at index i."
#define G_AST_NODELIST_REF_SPECIALIZED_SIG s7_make_signature(s7, 3, s7_t(s7), s7_make_symbol(s7, "ast_nodelist?"), s7_make_symbol(s7, "integer?"))

static s7_pointer g_ast_nodelist_ref_specialized(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelist_ref_specialized");
    /* debug_print_s7(s7, "g_ast_nodelist_ref_specialized args: ", args); */
#endif

    UT_array *nl;
    s7_int typ;
    s7_pointer obj;
    if (s7_list_length(s7, args) != 2)
        return(s7_wrong_number_of_args_error(s7, "ast-nodelist-ref takes 2 arguments: ~~S", args));

    obj = s7_car(args);
    typ = s7_c_object_type(obj);
    if (typ != ast_nodelist_t)
        return(s7_wrong_type_arg_error(s7, "ast-nodelist-ref", 1, obj, "an ast-nodelist"));

    nl  = (UT_array *)s7_c_object_value(obj);

    if (s7_is_null(s7, s7_cdr(args)))
        return(s7_wrong_type_arg_error(s7, "ast-nodelist-ref", 1, obj, "missing index arg"));

    s7_pointer arg = s7_cadr(args);
    if (s7_is_integer(arg))
        return _ast_nodelist_lookup(s7, nl, arg);
    else {
        return(s7_wrong_type_arg_error(s7, "ast-nodelist-ref",
                                       2, arg, "an integer"));
    }
}

/** g_ast_nodelist_object_applicator

    (more accurate: object_applicator, not essentially tied to ref)

    registered by s7_c_type_set_ref

    first arg is "self"

    function called when objects of type ast_nodelist are evaluated as
    functions, i.e. when they occur in function position (car of a
    list).

    not to be confused with generic ref of SRFI 123, e.g. (ref vec i),
    which s7 does not support.(?)

    by convention, same as ref_specialized (i.e ast_nodelist-ref) but this
    is not a requirement. could be used for anything, not just
    reference. example: (o :child-count) == (ast_nodelist-child-count o)
    or (o :fullname) concats (o :fname) and (o :lname)

    iow, it's a generic generic function, whereas generic ref is just
    a generic ref function. "meta generic?"

    in practice, its a dispatcher. sorta. its job is to inspect the args and
    decide what to do with them.
 */
static s7_pointer g_ast_nodelist_object_applicator(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelist_object_applicator");
    debug_print_s7(s7, "APPLICATOR ARGS: ", s7_cdr(args));
#endif

    /* no need to check arg1, it's the "self" ast_nodelist obj */
    /* s7_pointer g  = (struct node_s *)s7_c_object_value(obj); */

    s7_pointer rest = s7_cdr(args);
    if (s7_is_null(s7, rest))
        return(s7_wrong_type_arg_error(s7, "ast_nodelist-fn",
                                       1, rest, "missing keyword arg"));

    s7_pointer op = s7_car(rest);

    /* Currently s7 does not make a distinction between keywords and symbols; (keyword? :a) and (symbol? :a) both report true, and s7_is_ */
    if (s7_is_keyword(op)) {
        if (op == s7_make_keyword(s7, "subnodes_ct")) {
            log_debug("running :subnodes_ct");
            s7_pointer obj = s7_car(args);
            struct node_s *cs  = (struct node_s *)s7_c_object_value(obj);
            int ct = utarray_len(cs->subnodes);
            return s7_make_integer(s7, ct);
        } else {
            return g_ast_nodelist_ref_specialized(s7, args);
        }
    } else {
        if (s7_is_symbol(op)) { /* method access ((ast_nodelist 'foo) b) etc */
            s7_pointer val;
            val = s7_symbol_local_value(s7, op, g_ast_nodelist_methods_let);
            if (val != op)
                return(val);
            else {
                /* found self-referring method? method not found? methods table corrupt */
                /* return(s7_wrong_type_arg_error(s7, "ast_nodelist-ref", */
                /*                                2, arg, "a kw or sym")); */
                log_error("ERROR: corrupt object-methods-let?");
                return NULL;
            }
	} else {
            return(s7_wrong_type_arg_error(s7, "ast_nodelist-ref",
                                           2, op, "a keyword or symbol"));
        }
    }
}

/* **************** */
/** g_ast_nodelist_set_specialized

    registered twice: as a c-type generalize set! (s7_c_type_set_set()) and
    as procedure "ast_nodelist-set!" (s7_define_typed_function())

    generalized set: (set! (c-obj :k) v)

    in this case set! will call the set method registered with the
    c-obj's c-type, passing the c-obj, key :k, and value v.

    note that outside of this set! context, (c-obj :k) will lookup the
    value bound to :k in c-obj (using g_struct_get).
 */
#define G_AST_NODELIST_SET_SPECIALIZED_HELP "(ast_nodelist-set! b i x) sets the ast_nodelist value at index i to x."

#define G_AST_NODELIST_SET_SPECIALIZED_SIG s7_make_signature(s7, 4, s7_make_symbol(s7, "float?"), s7_make_symbol(s7, "ast_nodelist?"), s7_make_symbol(s7, "integer?"), s7_make_symbol(s7, "float?"))

static s7_pointer _update_ast_nodelist(s7_scheme *s7,
                                  struct node_s *ast_nodelist,
                                  s7_pointer key, s7_pointer val)
{
#ifdef DEBUG_TRACE
    log_debug("_update_ast_nodelist");
#endif

    if (key == s7_make_keyword(s7, "type")) {
        if (!s7_is_integer(val))
            return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!",
                                           3, val, "an integer"));
        ast_nodelist->type = s7_integer(val);
        return val;
    }

    if (key == s7_make_keyword(s7, "line")) {
        if (!s7_is_integer(val))
            return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!",
                                           3, val, "an integer"));
        ast_nodelist->line = s7_integer(val);
        return val;
    }

    if (key == s7_make_keyword(s7, "col")) {
        if (!s7_is_integer(val))
            return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!",
                                           3, val, "an integer"));
        ast_nodelist->col = s7_integer(val);
        return val;
    }

    if (key == s7_make_keyword(s7, "trailing_newline")) {
        if (!s7_is_boolean(val))
            return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!",
                                           3, val, "a boolean"));
        ast_nodelist->trailing_newline = s7_boolean(s7, val);
        return val;
    }

    if (key == s7_make_keyword(s7, "qtype")) {
        if (!s7_is_integer(val))
            return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!",
                                           3, val, "an integer"));
        ast_nodelist->qtype = s7_integer(val);
        return val;
    }

    if (key == s7_make_keyword(s7, "s")) {
        if (!s7_is_string(val))
            return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!",
                                           3, val, "a string"));
        free(ast_nodelist->s);
        /* s7_string() doc says "do not free the string", so we need
           to copy it? */
        int n = s7_string_length(val);
        ast_nodelist->s = calloc(1, n);
        strncpy(ast_nodelist->s, s7_string(val), n);
        return val;
    }

    //FIXME: implement for ast_nodelistlist?
    // we need this if we want to be able to replace subnodes/comments
    // e.g. replace a deps list

    /* if (key == s7_make_keyword(s7, "comments")) { */
    /*     if (s7_c_object_type(val) != ast_nodelist_t) { */
    /*         return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!", */
    /*                                        3, val, "a ast_nodelist")); */
    /*     } else { */
    /*         log_debug("2 xxxxxxxxxxxxxxxx"); */
    /*         ast_nodelist->substruct = s7_c_object_value(val); */
    /*         return val; */
    /*     } */
    /* } */

    /* if (key == s7_make_keyword(s7, "subnodes")) { */
    /*     if (s7_c_object_type(val) != ast_nodelist_t) { */
    /*         return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!", */
    /*                                        3, val, "a ast_nodelist")); */
    /*     } else { */
    /*         log_debug("2 xxxxxxxxxxxxxxxx"); */
    /*         ast_nodelist->substruct = s7_c_object_value(val); */
    /*         return val; */
    /*     } */
    /* } */

  return(s7_error(s7, s7_make_symbol(s7, "key not found"),
                  s7_list(s7, 2, s7_make_string(s7, "key not found: ~S"),
                          key)));
}

/* g_ast_nodelist_set_specialized

   implements (ast_nodelist_set! ...)

   what about (set! (node :subnodes) nodelist)? we need ctype for nodelist
 */
static s7_pointer g_ast_nodelist_set_specialized(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelist_set_specialized");
#endif
    struct node_s *g;
    s7_int typ;
    s7_pointer obj, kw;

    /* set! methods/procedures need to check that they have
       been passed the correct number of arguments: 3 */

    if (s7_list_length(s7, args) != 3)
        return(s7_wrong_number_of_args_error(s7, "ast_nodelist-set! takes 3 arguments: ~~S", args));

    obj = s7_car(args);
    /* qua procedure, check type of first arg */
    typ = s7_c_object_type(obj);
    if (typ != ast_nodelist_t)
        return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!", 1, obj, "a ast_nodelist"));

    if (s7_is_immutable(obj))
        return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!", 1, obj, "a mutable ast_nodelist"));

    /* validate lookup key type - in this case, a keyword */
    kw = s7_cadr(args);
    if (!s7_is_keyword(kw))
        return(s7_wrong_type_arg_error(s7, "ast_nodelist-set!",
                                       2, kw, "a keyword"));

    /* mutate to object: */
    g = (struct node_s *)s7_c_object_value(obj);
    _update_ast_nodelist(s7, g, kw, s7_caddr(args));
    //FIXME: r7rs says result of set! is unspecified. does that mean
    //implementation-specified?
    return s7_unspecified(s7);
}

static s7_pointer g_ast_nodelist_set_generic(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelist_set_generic");
    /* debug_print_s7(s7, "set_generic spec: ", args); */
#endif
    return g_ast_nodelist_set_specialized(s7, args);
}

static void _register_get_and_set(s7_scheme *s7)
{
    /* generic (SRFI 17) */
    s7_c_type_set_getter(s7, ast_nodelist_t, s7_name_to_value(s7, "ast_nodelist-ref"));
    s7_c_type_set_setter(s7, ast_nodelist_t, s7_name_to_value(s7, "ast_nodelist-set!"));
    // only for var setters, not fn setters?
    /* s7_set_setter(s7, */
    /*               s7_name_to_value(s7, "ast_nodelist-ref"), */
    /*               /\* s7_make_symbol(s7, "ast_nodelist-ref"), *\/ */
    /*               s7_name_to_value(s7, "ast_nodelist-set!")); */

    /* set_ref should be called set_applicator or some such */
    s7_c_type_set_ref(s7, ast_nodelist_t, g_ast_nodelist_object_applicator);
    s7_c_type_set_set(s7, ast_nodelist_t, g_ast_nodelist_set_generic);
}

/* /section: getters and setters */

/* **************************************************************** */
/* section: display */
char *g_ast_nodelist_display(s7_scheme *s7, void *value)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelist_display");
#endif

    UT_array *nl = (UT_array*)value;

    char buf[128];
    int len;

    sprintf(buf, "#ast_nodelist<\n");
    len = strlen(buf);
    snprintf(display_ptr, len+1, "%s", buf);
    display_ptr += len;

    struct node_s *n=NULL;
    while( (n=(struct node_s*)utarray_next(nl, n)) ) {
        g_ast_node_display(s7, n); /* updates global display_buf */
    }
    /* sprintf(buf, "BAR\n"); */
    /* len = strlen(buf); */
    /* snprintf(display_ptr, len+1, "%s", buf); */
    /* display_ptr += len; */

    sprintf(display_ptr - 2, ">"); /* backup to rm , and newline */
    /* len = strlen(buf); */
    /* snprintf(display_ptr, len+1, "%s", buf); */
    display_ptr -= 1;

    return display_buf;
}

/** g_ast_nodelist_display_readably

    produces a "roundtrippable" string, one that when read by the reader
    results in an object equal to the original.
 */
char *g_ast_nodelist_display_readably(s7_scheme *s7, void *value)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelist_display_readably");
#endif

    UT_array *nl = (UT_array *)value;

    // check display_buf size, expand if needed

    sprintf(display_ptr++, "(");

    struct node_s *n=NULL;
    while( (n=(struct node_s*)utarray_next(nl, n)) ) {
        g_ast_node_display_readably(s7, n); /* updates global display_buf */
    }

    sprintf(display_ptr - 1, ")");  /* backup to rem last newline */
    /* display_ptr += 2; */

    return display_buf;
}

/* /section: display */

/* **************************************************************** */
/* section: serialization */
static s7_pointer g_ast_nodelist_to_string(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelist_to_string");
    /* debug_print_s7(s7, "to_string cdr: ", s7_cdr(args)); */
#endif

    s7_pointer obj, choice;
    char *descr;
    obj = s7_car(args);
    if (s7_is_pair(s7_cdr(args)))
        choice = s7_cadr(args);
    else choice = s7_t(s7);
    if (choice == s7_make_keyword(s7, "readable"))
        descr = g_ast_nodelist_display_readably(s7, s7_c_object_value(obj));
    else descr = g_ast_nodelist_display(s7, s7_c_object_value(obj));
    /* printf("g_ast_nodelist_display => %s", descr); */
    obj = s7_make_string(s7, descr);

    free(descr); //BUG? FIXME free substruct strings
    return(obj);
}
/* /section: serialization */

/* **************************************************************** */
/* section: c-object construction */

/** g_ast_nodelist_copy

    registered as c-type method using s7_c_type_set_copy

    this extends the predefined (copy ...) proc. to handle our custom
    c-type.

    in s7, 'copy' is generic, but it seems to be limited to sequences;
    maybe copy-seq would be better. OTOH it does work with e.g. a
    single int, as in (let* ((a 1) (b (copy a))) ...). But the two-arg
    version (copy a b) only works with sequences?

    But it's also not strictly copy; it may copy only part of the
    source, for example. And it's destructive. A better name might be
    update! or similar. (idiomatic english: "copy a to b" means "make
    b distinct from but identical to a".)

    if arg1 has ast_nodelist type
        if cdr(args) is pair
            arg2 = cadr(args)
            check for mutability
            if arg2.type is ast_nodelist_t
                ok to copy
                if optional 3rd arg ('start-cp-from' index)
                    ...
                else
                    copy arg1 data to arg1
            else ;; arg2.type is NOT ast_nodelist_t
                g_block_copy assumes arg2 is a vector and copies data to it?
        else ;; only one arg, copy it
            make a new ast_nodelist
            copy arg1 data to new ast_nodelist
            return new ast_nodelist
    else ;;  arg1 not ast_nodelist, so arg2 must be

 */
static s7_pointer g_ast_nodelist_copy(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_ast_nodelist_copy");
    debug_print_s7(s7, "COPY ARGS: ", args);
    /* debug_print_s7(s7, "COPY ARGS len: ", s7_list_length(s7, args)); */
#endif

    s7_pointer arg1=NULL, arg2=NULL;
    UT_array *nl1=NULL, *nl2=NULL; /* arrays of struct ast_node_s */

    arg1 = s7_car(args);
    if (s7_c_object_type(arg1) == ast_nodelist_t) {
        log_debug("copy ast_nodelist");
        nl1 = (UT_array*)s7_c_object_value(arg1);
        if (s7_is_pair(s7_cdr(args))) {
            log_debug("copy a b");
            arg2 = s7_cadr(args);
            if (s7_is_immutable(arg2))
                return(s7_wrong_type_arg_error(s7, "ast_nodelist-copy!",
                                               0, arg2, "a mutable ast_nodelist"));
            if (s7_c_object_type(arg2) == ast_nodelist_t) {
                log_debug("copy ast_nodelist to ast_nodelist, destructively");
                nl2 = (UT_array*)s7_c_object_value(arg2);
                nodelist_copy_destructively(nl2, nl1);
                return arg2;
            } else {
                return(s7_wrong_type_arg_error(s7, "ast_nodelist-copy!",
                                               0, arg2, "a mutable ast_nodelist"));
            }
        } else {
            log_debug("copy one");
            /* only one arg, copy it to new ast_nodelist */
            nodelist_new(nl2);
            int rc = nodelist_copy(nl2, nl1); // in lib/starlark/nodes.c
            if (rc != 0) {
                // this should never happen...
                log_debug("FIXME: throw s7 error...");
                exit(EXIT_FAILURE);
            }
            return s7_make_c_object(s7, ast_nodelist_t,
                                    (void *)nl2);
        }
    } else {
        //FIXME: implement
        log_debug("copy non-nodelist");
        /* arg1 type != ast_nodelist_t */
    }
}

/** g_new_ast_nodelist
 */
/* docstring passed to the s7_define_.. used to register the fn in Scheme */
#define G_NEW_AST_NODELIST_HELP "(make-ast-nodelist) returns a new ast_nodelist"

static s7_pointer g_new_ast_nodelist(s7_scheme *s7, s7_pointer args)
{
#ifdef DEBUG_TRACE
    log_debug("g_new_ast_nodelist");
#endif

    UT_array *new_ast_nodelist = nodelist_new();

    s7_pointer new_ast_nodelist_s7 = s7_make_c_object(s7, ast_nodelist_t,
                                                      (void *)new_ast_nodelist);

    _register_c_object_methods(s7, new_ast_nodelist_s7);

    return(new_ast_nodelist_s7);
}
/* /section: c-object construction */

/* **************************************************************** */
/* section: c-object destruction */
static s7_pointer g_destroy_ast_nodelist(s7_scheme *s7, s7_pointer obj)
{
#ifdef DEBUG_TRACE
    log_debug("g_destroy_ast_nodelist");
#endif
    UT_array *nl = (UT_array*)s7_c_object_value(obj);
    nodelist_free(nl);
    return NULL;
}
/* /section: c-object destruction */

/* section: extension methods */
/* extension methods extend standard Scheme procedures like 'length'
   and 'equals' to support custom c-types.

   unsupported methods return #f (?)
 */

#define METHODS_PREFIX   "(openlet (immutable! (inlet "
#define METHODS_POSTFIX  ")))"

#define OBJECT_METHODS \
    "'float-vector? (lambda (p) (display \"foo \") #t) " \
    "'signature (lambda (p) (list '#t 'ast_nodelist? 'integer?)) " \
    "'type ast_nodelist? " \
    "'foo (lambda (self) \"hello from foo method!\") " \
    "'memq (lambda (self arg) \"hello from perverse memq method!\") " \
    "'arity (lambda (p) (cons 1 1)) " \
    "'aritable? (lambda (p args) (= args 1)) " \
    "'vector-dimensions (lambda (p) (list (length p))) " \
    "'empty (lambda (p) (zero? (length p))) " \
    "'ref ast_nodelist-ref " \
    "'vector-ref ast_nodelist-ref " \
    "'vector-set! ast_nodelist-set! "
    /* "'reverse! ast_nodelist-reverse! " \ */
    /* "'subsequence subast_nodelist " \ */
    /* "'append ast_nodelist-append " */

/* object methods: registered on each object, not type */
static void _register_c_object_methods(s7_scheme *s7, s7_pointer ast_nodelist)
{
#ifdef DEBUG_TRACE
    log_debug("_register_c_object_methods");
#endif
    static bool initialized = false;
    if (!initialized) {
        g_ast_nodelist_methods_let = s7_eval_c_string(s7, METHODS_PREFIX OBJECT_METHODS METHODS_POSTFIX);
        s7_gc_protect(s7, g_ast_nodelist_methods_let);
        initialized = true;
    }
    s7_c_object_set_let(s7, ast_nodelist, g_ast_nodelist_methods_let);
    s7_openlet(s7, ast_nodelist);
}

static void _register_c_type_methods(s7_scheme *s7, s7_int ast_nodelist_t)
{
#ifdef DEBUG_TRACE
    log_debug("_register_c_type_methods");
#endif
    s7_c_type_set_gc_free(s7, ast_nodelist_t, g_destroy_ast_nodelist);
    s7_c_type_set_gc_mark(s7, ast_nodelist_t, g_ast_nodelist_gc_mark);

    /* s7_c_type_set_equal(s7, ast_nodelist_t, _ast_nodelists_are_c_eql); */
    s7_c_type_set_is_equal(s7, ast_nodelist_t, g_ast_nodelists_are_equal);
    s7_c_type_set_is_equivalent(s7, ast_nodelist_t, g_ast_nodelists_are_equivalent);

    s7_c_type_set_copy(s7, ast_nodelist_t, g_ast_nodelist_copy);
    /* s7_c_type_set_length(s7, ast_nodelist_t, g_ast_nodelist_length); */
    /* s7_c_type_set_reverse(s7, ast_nodelist_t, g_ast_nodelist_reverse); */
    /* s7_c_type_set_fill(s7, ast_nodelist_t, g_ast_nodelist_fill); */

    s7_c_type_set_to_string(s7, ast_nodelist_t, g_ast_nodelist_to_string);

}

/* /section: extension methods */


/* **************************************************************** */
/* section: c-type configuration */

static void _register_ast_nodelist_fns(s7_scheme *s7)
{
#ifdef DEBUG_TRACE
    log_debug("_register_ast_nodelist_fns");
#endif
    s7_define_safe_function(s7, "make-ast-nodelist",
                            g_new_ast_nodelist,
                            0, 0, true, G_NEW_AST_NODELIST_HELP);

    s7_define_typed_function(s7, "ast-nodelist?", g_is_ast_nodelist, 1, 0, false, g_is_ast_nodelist_help, g_is_ast_nodelist_sig);

    /* specialized get/set! */
    s7_define_typed_function(s7, "ast-nodelist-ref", g_ast_nodelist_ref_specialized, 2, 0, false, G_AST_NODELIST_REF_SPECIALIZED_HELP, G_AST_NODELIST_REF_SPECIALIZED_SIG);
    s7_define_typed_function(s7, "ast-nodelist-set!", g_ast_nodelist_set_specialized, 3, 0, false, G_AST_NODELIST_SET_SPECIALIZED_HELP, G_AST_NODELIST_SET_SPECIALIZED_SIG);

    // ast_nodelist-let => s7_c_object_let, a let for the instance not the type
    /* s7_define_safe_function(s7, "ast_nodelist-let", g_ast_nodelist_let, 1, 0, false, g_ast_nodelist_let_help); */

    /* s7_define_safe_function(s7, "subast_nodelist", g_subast_nodelist, 1, 0, true, g_subast_nodelist_help); */
    /* s7_define_safe_function(s7, "ast_nodelist-append", g_ast_nodelist_append, 0, 0, true, g_ast_nodelist_append_help); */
    /* s7_define_safe_function(s7, "ast_nodelist-reverse!", g_ast_nodelist_reverse_in_place, 1, 0, false, g_ast_nodelist_reverse_in_place_help); */
}

/* **************** */
/** configure_s7_ast_nodelist_type(s7_scheme *s7)

    public
*/
//FIXME: return int rc?
EXPORT int configure_s7_ast_nodelist_type(s7_scheme *s7)
{
#ifdef DEBUG_TRACE
    log_debug("configure_s7_ast_nodelist_type");
#endif
    /* s7_int t = _make_c_type(s7); */
    ast_nodelist_t = s7_make_c_type(s7, "<ast_nodelist>");
    _register_c_type_methods(s7, ast_nodelist_t);
    _register_get_and_set(s7);
    _register_ast_nodelist_fns(s7);
    s7_provide(s7, "ast_nodelist");
    return ast_nodelist_t;
}
/* section: c-type configuration */

/* **************************************************************** */
/* section: gc */
static s7_pointer g_ast_nodelist_gc_mark(s7_scheme *s7, s7_pointer p)
{
    /* nothing to mark because we protect g_ast_nodelist_methods_let, and all ast_nodelist objects get the same let */
    return(p);
}

/* static s7_pointer g_ast_nodelist_gc_free(s7_scheme *s7, s7_pointer obj) */
/* { */
/* #ifdef DEBUG_TRACE */
/*     log_debug("g_ast_nodelist_gc_free"); */
/* #endif */
/*     struct node_s *cs = (UT_array*)s7_c_object_value(obj); */
/*     ast_nodelist_free(cs); */
/*     return(NULL); */
/* } */

/* /section: gc */
