/**
 *     Copyright 2016-2019 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef COUCHBASE_H_
#define COUCHBASE_H_

// clang-format off
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>

#include <libcouchbase/couchbase.h>
#include <libcouchbase/ixmgmt.h>
#include <libcouchbase/crypto.h>

#include <ext/standard/base64.h>
#include <ext/standard/php_var.h>
#include <zend_smart_str.h>
#include <zend_interfaces.h>
#include <zend_exceptions.h>

#include <ext/json/php_json.h>

#include "php_couchbase.h"
#include "log.h"
#include "contrib/php_array.h"
#include <string.h>
// clang-format on

#define PCBC_ARG_VARIADIC_INFO(__pcbc_pass_by_ref, __pcbc_name)                                                        \
    ZEND_ARG_VARIADIC_INFO((__pcbc_pass_by_ref), (__pcbc_name))

enum pcbc_constants {
    PERSISTTO_ONE = 1,
    PERSISTTO_TWO = 2,
    PERSISTTO_THREE = 4,
    PERSISTTO_MASTER = PERSISTTO_ONE,
    REPLICATETO_ONE = 1 << 4,
    REPLICATETO_TWO = 2 << 4,
    REPLICATETO_THREE = 4 << 4
};

struct pcbc_connection {
    lcb_INSTANCE_TYPE type;
    char *connstr;
    char *bucketname;
    char *username;
    lcb_INSTANCE *lcb;
    int refs;
    time_t idle_at;
};
typedef struct pcbc_connection pcbc_connection_t;
lcb_STATUS pcbc_connection_get(pcbc_connection_t **result, lcb_INSTANCE_TYPE type, const char *connstr,
                               const char *bucketname, const char *username, const char *password TSRMLS_DC);
void pcbc_connection_addref(pcbc_connection_t *conn TSRMLS_DC);
void pcbc_connection_delref(pcbc_connection_t *conn TSRMLS_DC);
void pcbc_connection_cleanup();

ZEND_BEGIN_MODULE_GLOBALS(couchbase)
char *log_level;

char *enc_format;
char *enc_cmpr;
int enc_format_i;
int enc_cmpr_i;
long enc_cmpr_threshold;
long pool_max_idle_time;
double enc_cmpr_factor;
zend_bool dec_json_array;
ZEND_END_MODULE_GLOBALS(couchbase)
ZEND_EXTERN_MODULE_GLOBALS(couchbase)

#ifdef ZTS
#define PCBCG(v) TSRMG(couchbase_globals_id, zend_couchbase_globals *, v)
#else
#define PCBCG(v) (couchbase_globals.v)
#endif

#if PHP_VERSION_ID < 70200

#define php_base64_decode_str(arg) php_base64_decode((const unsigned char *)ZSTR_VAL(arg), ZSTR_LEN(arg))

#undef ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO
#define ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO(name, type, allow_null)                                                   \
    ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, 0, -1, type, NULL, allow_null)

#define zend_parse_parameters_none_throw zend_parse_parameters_none

#define ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO(name, class_name, allow_null)                                              \
    ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, 0, -1, IS_OBJECT, NULL, allow_null)

#define ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(name, return_reference, required_num_args, class_name, allow_null)      \
    ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, IS_OBJECT, NULL, allow_null)
#endif

#define PCBC_DATE_FORMAT_RFC3339 "Y-m-d\\TH:i:sP"

extern char *pcbc_client_string;
extern zend_class_entry *pcbc_bucket_ce;
extern zend_class_entry *pcbc_mutation_token_ce;
extern zend_class_entry *pcbc_mutation_state_ce;
extern zend_class_entry *pcbc_search_options_ce;
extern zend_class_entry *pcbc_search_query_ce;
extern zend_class_entry *pcbc_search_sort_ce;
extern zend_class_entry *pcbc_search_facet_ce;
extern zend_class_entry *pcbc_json_serializable_ce;
extern zend_class_entry *pcbc_crypto_provider_ce;

extern zend_class_entry *pcbc_collection_ce;
extern zend_class_entry *pcbc_binary_collection_ce;

#define PCBC_RESOLVE_COLLECTION_EX(class_entry)                                                                        \
    const char *collection_str = NULL, *scope_str = NULL;                                                              \
    size_t collection_len = 0, scope_len = 0;                                                                          \
    pcbc_bucket_t *bucket = NULL;                                                                                      \
    {                                                                                                                  \
        zval *prop, rv__;                                                                                              \
        zval *self = getThis();                                                                                        \
        prop = zend_read_property((class_entry), self, ZEND_STRL("bucket"), 0, &rv__);                                 \
        bucket = Z_BUCKET_OBJ_P(prop);                                                                                 \
        prop = zend_read_property((class_entry), self, ZEND_STRL("scope"), 0, &rv__);                                  \
        if (Z_TYPE_P(prop) == IS_STRING) {                                                                             \
            scope_str = Z_STRVAL_P(prop);                                                                              \
            scope_len = Z_STRLEN_P(prop);                                                                              \
        }                                                                                                              \
        prop = zend_read_property((class_entry), self, ZEND_STRL("name"), 0, &rv__);                                   \
        if (Z_TYPE_P(prop) == IS_STRING) {                                                                             \
            collection_str = Z_STRVAL_P(prop);                                                                         \
            collection_len = Z_STRLEN_P(prop);                                                                         \
        }                                                                                                              \
    }

#define PCBC_RESOLVE_COLLECTION PCBC_RESOLVE_COLLECTION_EX(pcbc_collection_ce)
#define PCBC_RESOLVE_BINARY_COLLECTION PCBC_RESOLVE_COLLECTION_EX(pcbc_binary_collection_ce)

void pcbc_create_lcb_exception(zval *return_value, long code, zend_string *context, zend_string *ref, int http_code,
                               const char *http_msg TSRMLS_DC);

void pcbc_exception_init(zval *return_value, long code, const char *message TSRMLS_DC);
#define throw_pcbc_exception(__pcbc_message, __pcbc_code)                                                              \
    do {                                                                                                               \
        zval __pcbc_error;                                                                                             \
        ZVAL_UNDEF(&__pcbc_error);                                                                                     \
        pcbc_exception_init(&__pcbc_error, __pcbc_code, __pcbc_message TSRMLS_CC);                                     \
        zend_throw_exception_object(&__pcbc_error TSRMLS_CC);                                                          \
    } while (0)

#define throw_lcb_exception(code, result_ce)                                                                           \
    do {                                                                                                               \
        zend_string *ctx = NULL, *ref = NULL;                                                                          \
        zval *zref, __rv1, *zctx, __rv2;                                                                               \
        if (result_ce) {                                                                                               \
            zref = zend_read_property(result_ce, return_value, ZEND_STRL("err_ref"), 0, &__rv1);                       \
            if (Z_TYPE_P(zref) == IS_STRING) {                                                                         \
                ref = Z_STR_P(zref);                                                                                   \
            }                                                                                                          \
            zctx = zend_read_property(result_ce, return_value, ZEND_STRL("err_ctx"), 0, &__rv2);                       \
            if (Z_TYPE_P(zctx) == IS_STRING) {                                                                         \
                ctx = Z_STR_P(zctx);                                                                                   \
            }                                                                                                          \
        }                                                                                                              \
        zval __pcbc_error;                                                                                             \
        ZVAL_UNDEF(&__pcbc_error);                                                                                     \
        pcbc_create_lcb_exception(&__pcbc_error, code, ctx, ref, 0, NULL TSRMLS_CC);                                   \
        zend_throw_exception_object(&__pcbc_error TSRMLS_CC);                                                          \
    } while (0)

#define throw_http_exception(code, query_code, query_msg)                                                              \
    do {                                                                                                               \
        zval __pcbc_error;                                                                                             \
        ZVAL_UNDEF(&__pcbc_error);                                                                                     \
        pcbc_create_lcb_exception(&__pcbc_error, code, NULL, NULL, query_code, query_msg TSRMLS_CC);                   \
        zend_throw_exception_object(&__pcbc_error TSRMLS_CC);                                                          \
    } while (0)

#define PCBC_CONTENT_TYPE_FORM "application/x-www-form-urlencoded"
#define PCBC_CONTENT_TYPE_JSON "application/json"

#define PCBC_ARRAY_MERGE(__pcbc_dest, __pcbc_src) php_array_merge((__pcbc_dest), (__pcbc_src)TSRMLS_CC)

#define ADD_NEXT_INDEX_STRING(zv, value) add_next_index_string(zv, value);

#define PCBC_ALLOC_OBJECT_T(obj_t, class_type)                                                                         \
    (obj_t *)ecalloc(1, sizeof(obj_t) + zend_object_properties_size(class_type))

#define PCBC_CE_DISABLE_SERIALIZATION(ce)                                                                              \
    do {                                                                                                               \
        ce->serialize = zend_class_serialize_deny;                                                                     \
        ce->unserialize = zend_class_unserialize_deny;                                                                 \
    } while (0);

#define PCBC_STRLEN_P(__pcbc_zval) Z_STRLEN((__pcbc_zval))
#define PCBC_STRVAL_P(__pcbc_zval) Z_STRVAL((__pcbc_zval))
#define PCBC_STRLEN_ZP(__pcbc_zval) Z_STRLEN_P((__pcbc_zval))
#define PCBC_STRVAL_ZP(__pcbc_zval) Z_STRVAL_P((__pcbc_zval))
#define PCBC_STRINGL(__pcbc_zval, __pcbc_str, __pcbc_len) ZVAL_STRINGL(&(__pcbc_zval), (__pcbc_str), (__pcbc_len))
#define PCBC_STRINGS(__pcbc_zval, __pcbc_smart_str)                                                                    \
    ZVAL_STRINGL(&(__pcbc_zval), ZSTR_VAL((__pcbc_smart_str).s), ZSTR_LEN((__pcbc_smart_str).s))
#define PCBC_STRING(__pcbc_zval, __pcbc_str) ZVAL_STRING(&(__pcbc_zval), (__pcbc_str))

#define pcbc_make_printable_zval(__pcbc_expr, __pcbc_expr_copy, __pcbc_use_copy)                                       \
    do {                                                                                                               \
        *(__pcbc_use_copy) = zend_make_printable_zval((__pcbc_expr), (__pcbc_expr_copy));                              \
    } while (0)

/* classes */
#define PCBC_JSON_RESET_STATE                                                                                          \
    do {                                                                                                               \
        JSON_G(error_code) = 0;                                                                                        \
        JSON_G(encode_max_depth) = PHP_JSON_PARSER_DEFAULT_DEPTH;                                                      \
    } while (0)

#define PCBC_JSON_ENCODE(__pcbc_buf, __pcbc_value, __pcbc_flags, __pcbc_error_code)                                    \
    do {                                                                                                               \
        PCBC_JSON_RESET_STATE;                                                                                         \
        php_json_encode((__pcbc_buf), (__pcbc_value), (__pcbc_flags)TSRMLS_CC);                                        \
        (__pcbc_error_code) = JSON_G(error_code);                                                                      \
    } while (0)

#define PCBC_JSON_COPY_DECODE(__pcbc_zval, __pcbc_src, __pcbc_len, __options, __pcbc_error_code)                       \
    do {                                                                                                               \
        char *__copy = estrndup((__pcbc_src), (__pcbc_len));                                                           \
        PCBC_JSON_RESET_STATE;                                                                                         \
        php_json_decode_ex((__pcbc_zval), (__copy), (__pcbc_len), (__options),                                         \
                           PHP_JSON_PARSER_DEFAULT_DEPTH TSRMLS_CC);                                                   \
        efree(__copy);                                                                                                 \
        (__pcbc_error_code) = JSON_G(error_code);                                                                      \
    } while (0)

#define PCBC_SMARTSTR_DUP(__pcbc_smart_str, __pcbc_receiver_buf)                                                       \
    do {                                                                                                               \
        (__pcbc_receiver_buf) = estrndup(ZSTR_VAL((__pcbc_smart_str).s), ZSTR_LEN((__pcbc_smart_str).s));              \
    } while (0)
#define PCBC_SMARTSTR_SET(__pcbc_smart_str, __pcbc_receiver_buf, __pcbc_receiver_length)                               \
    do {                                                                                                               \
        (__pcbc_receiver_buf) = ZSTR_VAL((__pcbc_smart_str).s);                                                        \
        (__pcbc_receiver_length) = ZSTR_LEN((__pcbc_smart_str).s);                                                     \
    } while (0)
#define PCBC_SMARTSTR_VAL(__pcbc_smart_str) (char *)ZSTR_VAL((__pcbc_smart_str).s)
#define PCBC_SMARTSTR_LEN(__pcbc_smart_str) ((__pcbc_smart_str).s ? (int)(ZSTR_LEN((__pcbc_smart_str).s)) : 0)
#define PCBC_SMARTSTR_EMPTY(__pcbc_smart_str) ((__pcbc_smart_str).s == NULL || PCBC_SMARTSTR_LEN(__pcbc_smart_str) == 0)

typedef struct {
    char *username;
    int username_len;
    char *password;
    int password_len;
    zend_object std;
} pcbc_password_authenticator_t;

typedef struct {
    zend_object std;
} pcbc_cert_authenticator_t;

typedef struct {
    char *connstr;
    char *username;
    char *password;
    pcbc_connection_t *conn;
    zend_object std;
} pcbc_cluster_t;

struct pcbc_crypto_id {
    char *name;
    int name_len;
    struct pcbc_crypto_id *next;
};
typedef struct pcbc_crypto_id pcbc_crypto_id_t;

typedef struct {
    pcbc_connection_t *conn;
    zval encoder;
    zval decoder;
    lcb_BTYPE type;
    pcbc_crypto_id_t *crypto_head; /* registered crypto providers */
    pcbc_crypto_id_t *crypto_tail; /* registered crypto providers */
    zend_object std;
} pcbc_bucket_t;

typedef struct {
    pcbc_connection_t *conn;
    zend_object std;
} pcbc_query_index_manager_t;

typedef struct {
    void *next;
    lcb_STATUS err;
    char *err_ctx;
    size_t err_ctx_len;
    char *err_ref;
    size_t err_reflen;
} opcookie_res;

int pcbc_decode_value(zval *return_value, pcbc_bucket_t *bucket, const char *bytes, int bytes_len, uint32_t flags,
                      uint8_t datatype TSRMLS_DC);
int pcbc_encode_value(pcbc_bucket_t *bucket, zval *value, void **bytes, lcb_size_t *nbytes, lcb_uint32_t *flags,
                      uint8_t *datatype TSRMLS_DC);

void pcbc_http_request(zval *return_value, lcb_INSTANCE *conn, lcb_CMDHTTP *cmd, int json_response, void *cbctx,
                       void(httpcb)(void *, zval *, zval *), int(errorcb)(void *, zval *) TSRMLS_DC);

void pcbc_query_index_manager_init(zval *return_value, zval *cluster TSRMLS_DC);

void pcbc_mutation_state_export_for_n1ql(zval *obj, zval *scan_vectors TSRMLS_DC);
void pcbc_mutation_state_export_for_search(zval *mutation_state, zval *scan_vectors TSRMLS_DC);

void pcbc_crypto_register(pcbc_bucket_t *obj, const char *name, int name_len, zval *provider TSRMLS_DC);
void pcbc_crypto_unregister(pcbc_bucket_t *obj, const char *name, int name_len TSRMLS_DC);
void pcbc_crypto_encrypt_fields(pcbc_bucket_t *obj, zval *document, zval *options, const char *prefix,
                                zval *return_value TSRMLS_DC);
void pcbc_crypto_decrypt_fields(pcbc_bucket_t *obj, zval *document, zval *options, const char *prefix,
                                zval *return_value TSRMLS_DC);

static inline pcbc_cluster_t *pcbc_cluster_fetch_object(zend_object *obj)
{
    return (pcbc_cluster_t *)((char *)obj - XtOffsetOf(pcbc_cluster_t, std));
}
static inline pcbc_bucket_t *pcbc_bucket_fetch_object(zend_object *obj)
{
    return (pcbc_bucket_t *)((char *)obj - XtOffsetOf(pcbc_bucket_t, std));
}
static inline pcbc_password_authenticator_t *pcbc_password_authenticator_fetch_object(zend_object *obj)
{
    return (pcbc_password_authenticator_t *)((char *)obj - XtOffsetOf(pcbc_password_authenticator_t, std));
}
#define Z_CLUSTER_OBJ(zo) (pcbc_cluster_fetch_object(zo))
#define Z_CLUSTER_OBJ_P(zv) (pcbc_cluster_fetch_object(Z_OBJ_P(zv)))
#define Z_BUCKET_OBJ(zo) (pcbc_bucket_fetch_object(zo))
#define Z_BUCKET_OBJ_P(zv) (pcbc_bucket_fetch_object(Z_OBJ_P(zv)))
#define Z_PASSWORD_AUTHENTICATOR_OBJ(zo) (pcbc_password_authenticator_fetch_object(zo))
#define Z_PASSWORD_AUTHENTICATOR_OBJ_P(zv) (pcbc_password_authenticator_fetch_object(Z_OBJ_P(zv)))

typedef struct {
    opcookie_res *res_head;
    opcookie_res *res_tail;
    lcb_STATUS first_error;
    int json_response;
    int json_options;
    zval exc;
    lcbtrace_SPAN *span;
} opcookie;

opcookie *opcookie_init();
void opcookie_destroy(opcookie *cookie);
void opcookie_push(opcookie *cookie, opcookie_res *res);
lcb_STATUS opcookie_get_first_error(opcookie *cookie);
opcookie_res *opcookie_next_res(opcookie *cookie, opcookie_res *cur);

#define FOREACH_OPCOOKIE_RES(Type, Res, cookie)                                                                        \
    Res = NULL;                                                                                                        \
    while ((Res = (Type *)opcookie_next_res(cookie, (opcookie_res *)Res)) != NULL)

#define PCBC_ADDREF_P(__pcbc_zval) Z_TRY_ADDREF_P((__pcbc_zval))

#define PCBC_DELREF_P(__pcbc_zval) Z_TRY_DELREF_P((__pcbc_zval))

#define set_property_str(target, getter, class_entry, prop)                                                            \
    do {                                                                                                               \
        const char *data = NULL;                                                                                       \
        size_t ndata = 0;                                                                                              \
        getter(target, &data, &ndata);                                                                                 \
        if (ndata && data) {                                                                                           \
            zend_update_property_stringl(class_entry, return_value, ZEND_STRL(prop), data, ndata TSRMLS_CC);           \
        }                                                                                                              \
    } while (0);

#define set_property_num(type, getter, class_entry, prop)                                                              \
    do {                                                                                                               \
        type data = 0;                                                                                                 \
        getter(resp, &data);                                                                                           \
        zend_update_property_long(class_entry, return_value, ZEND_STRL(prop), data TSRMLS_CC);                         \
    } while (0);

#endif /* COUCHBASE_H_ */
