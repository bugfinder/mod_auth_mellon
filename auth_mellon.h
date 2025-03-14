/*
 *
 *   auth_mellon.h: an authentication apache module
 *   Copyright © 2003-2007 UNINETT (http://www.uninett.no/)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef MOD_AUTH_MELLON_H
#define MOD_AUTH_MELLON_H

#include "config.h"

#include <stdbool.h>
#include <strings.h>

#include <lasso/lasso.h>
#include <lasso/xml/saml-2.0/samlp2_authn_request.h>
#include <lasso/xml/saml-2.0/samlp2_logout_request.h>
#include <lasso/xml/saml-2.0/samlp2_response.h>
#include <lasso/xml/saml-2.0/saml2_assertion.h>
#include <lasso/xml/saml-2.0/saml2_attribute_statement.h>
#include <lasso/xml/saml-2.0/saml2_attribute.h>
#include <lasso/xml/saml-2.0/saml2_attribute_value.h>
#include <lasso/xml/saml-2.0/saml2_authn_statement.h>
#include <lasso/xml/saml-2.0/saml2_audience_restriction.h>
#include <lasso/xml/misc_text_node.h>
#include "lasso_compat.h"

/* The following are redefined in ap_config_auto.h */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#undef HAVE_TIMEGM              /* is redefined again in ap_config.h */

#include "apr_base64.h"
#include "apr_time.h"
#include "apr_strings.h"
#include "apr_shm.h"
#include "apr_md5.h"
#include "apr_file_info.h"
#include "apr_file_io.h"
#include "apr_xml.h"
#include "apr_lib.h"
#include "apr_fnmatch.h"
#include "apr_random.h"

#ifdef HAVE_apr_pescape_hex
#include "apr_escape.h"
#endif

#include "ap_config.h"
#include "ap_socache.h"
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "mod_ssl.h"
#include "util_mutex.h"

/* Backwards-compatibility helpers. */
#include "auth_mellon_compat.h"


#define ENV_ATTR_PREFIX "MELLON_"

/* Initializing an apr_time_t to 0x7fffffffffffffffLL yields an
 * iso 8601 time with 1 second precision of "294247-01-10T04:00:54Z"
 * this is 22 characters, +1 for null terminator. */
#define ISO_8601_BUF_SIZE 23
#define ISO_8601_FMT  "%FT%TZ"

/*
 * Most ISO 8601 parsing routines can only handle dates with a 4 digit
 * year. Even though apr_time_t can represent dates whose year > 9999
 * we limit the maximum apr_time_t to 9999-12-31T23:59:59Z to
 * accommodate the parsing routines, this date is sufficiently far
 * into the future we can expect it to be greater than any date/time
 * we see in actual data.
 */
#define MAX_APR_TIME_T 253402300799000000L

#define IS_NODE(node, node_name, node_ns)                       \
    (node->type == XML_ELEMENT_NODE &&                          \
     strcmp((const char *)node->name, node_name) == 0 &&        \
     strcmp((const char *)node->ns->href, node_ns) == 0)

#define FIRST_ATTR_VALUE(attr_values)           \
    (attr_values && attr_values->nelts ?        \
     APR_ARRAY_IDX(attr_values, 0, char *) :    \
     NULL)


#define SESSION_STATE_ENTRY_SIZE 65536
#define SESSION_STATE_NODE_NAME "MellonSessionState"
#define SESSION_STATE_VERSION "1.0"

#define SESSION_STATE_NS_PREFIX "amss"
#define SESSION_STATE_NS_HREF "mod_auth_mellon"

#define XSI_NIL "nil"

/* Internal error codes */
#define AM_ERROR_INVALID_PAOS_HEADER 1
#define AM_ERROR_MISSING_PAOS_HEADER 2
#define AM_ERROR_MISSING_PAOS_MEDIA_TYPE 3


#ifdef ENABLE_DIAGNOSTICS
typedef enum {
    AM_DIAG_FLAG_ENABLED       = (1 << 0),
    AM_DIAG_FLAG_DISABLE       = 0,
    AM_DIAG_FLAG_ENABLE_ALL    = ~0,
} am_diag_flags_t;
#endif


/* Disable SameSite Environment Value */
#define AM_DISABLE_SAMESITE_ENV_VAR "MELLON_DISABLE_SAMESITE"

/* Force setting SameSite to None */
#define AM_FORCE_SAMESITE_NONE_NOTE "MELLON_FORCE_SAMESITE_NONE"


/* This is the length of the id we use (for session IDs and
 * replaying POST data).
 */
#define AM_ID_LENGTH 32

#define MEDIA_TYPE_PAOS "application/vnd.paos+xml"

#define am_get_srv_cfg(s) (am_srv_cfg_rec *)ap_get_module_config((s)->module_config, &auth_mellon_module)

#define am_get_mod_cfg(s) (am_get_srv_cfg((s)))->mc

#define am_get_dir_cfg(r) (am_dir_cfg_rec *)ap_get_module_config((r)->per_dir_config, &auth_mellon_module)

#define am_get_req_cfg(r) (am_req_cfg_rec *)ap_get_module_config((r)->request_config, &auth_mellon_module)

#ifdef ENABLE_DIAGNOSTICS
#define am_get_diag_cfg(s) (&(am_get_srv_cfg((s)))->diag_cfg)
#endif

typedef struct am_mod_cfg_rec {
    const char *post_dir;
    apr_time_t post_ttl;
    int post_count;
    apr_size_t post_size;

    /* These variables can't be allowed to change after the session store
     * has been initialized. Therefore we copy them before initializing
     * the session store.
     */
    apr_global_mutex_t *socache_lock;
    const char *socache_provider_name;
    const char *socache_provider_args;
    ap_socache_provider_t *socache_provider;
    ap_socache_instance_t *socache_instance;
    int socache_session_state_entry_size;
} am_mod_cfg_rec;


#ifdef ENABLE_DIAGNOSTICS
typedef struct am_diag_cfg_rec {
    const char *filename;
    apr_file_t *fd;
    am_diag_flags_t flags;
} am_diag_cfg_rec;
#endif

typedef struct am_srv_cfg_rec {
    am_mod_cfg_rec *mc;
#ifdef ENABLE_DIAGNOSTICS
    am_diag_cfg_rec diag_cfg;
#endif
} am_srv_cfg_rec;

typedef enum {
    am_enable_default,
    am_enable_off,
    am_enable_info,
    am_enable_auth
} am_enable_t;

typedef enum {
  am_samesite_default,
  am_samesite_lax,
  am_samesite_strict,
  am_samesite_none,
} am_samesite_t;

typedef enum {
    AM_COND_FLAG_NULL = 0x000, /* No flags */
    AM_COND_FLAG_OR   = 0x001, /* Or with  next condition */
    AM_COND_FLAG_NOT  = 0x002, /* Negate this condition */
    AM_COND_FLAG_REG  = 0x004, /* Condition is regex */
    AM_COND_FLAG_NC   = 0x008, /* Case insensitive match */
    AM_COND_FLAG_MAP  = 0x010, /* Try to use attribute name from MellonSetEnv */
    AM_COND_FLAG_REF  = 0x020, /* Set regex backreferences */
    AM_COND_FLAG_SUB  = 0x040, /* Substring match */

    /* The other options are internally used */
    AM_COND_FLAG_IGN  = 0x1000, /* Condition is to be ignored */
    AM_COND_FLAG_REQ  = 0x2000, /* Condition was set using MellonRequire */
    AM_COND_FLAG_FSTR = 0x4000, /* Value contains a format string */
} am_cond_flag_t;

extern const char *am_cond_options[];

/*
 * am_file_data_t is used to maintain information about a file:
 *
 * * The filesystem pathname
 * * Stat information about the file (e.g. type, size, times, etc.)
 * * If and when the file was stat'ed or read
 * * Error code of failed operation and error string description
 * * Contents of the file
 * * Flag indicating if contents were generated instead of being read
 *   from a file.
 */
typedef struct am_file_data_t {
    apr_pool_t *pool;     /* allocation pool */
    const char *path;     /* filesystem pathname, NULL for generated file */
    apr_time_t stat_time; /* when stat was performed, zero indicates never */
    apr_finfo_t finfo;    /* stat data */
    char *contents;       /* file contents */
    apr_time_t read_time; /* when contents was read, zero indicates never */
    apr_status_t rv;      /* most recent result value */
    const char *strerror; /* if rv is error then this is error description */
    bool generated;       /* true if contents generated instead of being
                             read from path */
} am_file_data_t;

typedef struct {
    const char *varname;
    int flags;
    const char *str; 
    ap_regex_t *regex; 
    const char *directive;
} am_cond_t;

typedef struct am_metadata {
    am_file_data_t *metadata; /* Metadata file with one or many IdP */
    am_file_data_t *chain;    /* Validating chain */
} am_metadata_t;

typedef struct am_dir_cfg_rec {
    /* enable_mellon is used to enable auth_mellon for a location.
     */
    am_enable_t enable_mellon;

    const char *varname;
    int secure;
    int http_only;
    const char *merge_env_vars;
    int env_vars_index_start;
    int env_vars_count_in_n;
    const char *cookie_domain;
    const char *cookie_path;
    am_samesite_t cookie_samesite;
    apr_array_header_t *cond;
    apr_hash_t *envattr;
    const char *env_prefix;
    const char *userattr;
    const char *idpattr;
    LassoSignatureMethod signature_method;
    int dump_session;
    int dump_saml_response;

    /* The "root directory" of our SAML2 endpoints. This path is relative
     * to the root of the web server.
     *
     * This path will always end with '/'.
     */
    const char *endpoint_path;

    /* Lasso configuration variables. */
    am_file_data_t *sp_metadata_file;
    am_file_data_t *sp_private_key_file;
    am_file_data_t *sp_cert_file;
    apr_array_header_t *idp_metadata;
    am_file_data_t *idp_ca_file;
    GList *idp_ignore;

    /* metadata autogeneration helper */
    char *sp_entity_id;
    apr_hash_t *sp_org_name;
    apr_hash_t *sp_org_display_name;
    apr_hash_t *sp_org_url;

    /* Maximum number of seconds a session is valid for. */
    int session_length;

    /* Maximum number of seconds a session idle timeout is valid for. */
    int session_idle_timeout;

    /* No cookie error page. */
    const char *no_cookie_error_page;

    /* Authorization error page. */
    const char *no_success_error_page;

    /* Login path for IdP initiated logins */
    const char *login_path;

    /* IdP discovery service */
    const char *discovery_url;
    int probe_discovery_timeout;
    apr_table_t *probe_discovery_idp;

    /* The configuration record we "inherit" the lasso server object from. */
    struct am_dir_cfg_rec *inherit_server_from;
    /* Mutex to prevent us from creating several lasso server objects. */
    apr_thread_mutex_t *server_mutex;

    /* AuthnContextClassRef list */
    apr_array_header_t *authn_context_class_ref;

    /* AuthnContextComparisonType */
    const char *authn_context_comparison_type;

    /* Controls the checking of SubjectConfirmationData.Address attribute */
    int subject_confirmation_data_address_check;

    /* MellonDoNotVerifyLogoutSignature idp set */
    apr_hash_t *do_not_verify_logout_signature;

    /* Controls whether the cache control header is set */
    int send_cache_control_header;

    /* Whether we should replay POST data after authentication. */
    int post_replay;

    /* Cached lasso server object. */
    LassoServer *server;

    /* Whether to send an ECP client a list of IdP's */
    int ecp_send_idplist;

    /* List of domains we can redirect to. */
    const char * const *redirect_domains;

    /* Enabled the session invalidate endpoint. */
    int enabled_invalidation_session;

    /* Send Expect Header. */
    int send_expect_header;

} am_dir_cfg_rec;

/* Bitmask for PAOS service options */
typedef enum {
    ECP_SERVICE_OPTION_CHANNEL_BINDING = 1,
    ECP_SERVICE_OPTION_HOLDER_OF_KEY = 2,
    ECP_SERVICE_OPTION_WANT_AUTHN_SIGNED = 4,
    ECP_SERVICE_OPTION_DELEGATION = 8,
} ECPServiceOptions;

typedef struct am_req_cfg_rec {
    char *cookie_value;
#ifdef HAVE_ECP
    bool ecp_authn_req;
    ECPServiceOptions ecp_service_options;
#endif /* HAVE_ECP */
#ifdef ENABLE_DIAGNOSTICS
    bool diag_emitted;
#endif
} am_req_cfg_rec;

typedef struct am_session_state_t {
    apr_pool_t *pool;    
    const char *session_id;
    LassoSaml2NameID *lasso_name_id;
    LassoSaml2NameID *issuer;
    LassoSaml2Assertion *lasso_assertion;
    apr_hash_t *env_attrs;
    apr_time_t expires;
    apr_time_t idle_timeout;
    int logged_in;
    const char *user;
    const char *cookie_token;
    const char *lasso_identity_dump;
    const char *lasso_session_dump;
    const char *saml_response;
} am_session_state_t;

/* Type for configuring environment variable names */
typedef struct am_envattr_conf_t {
    // Name of the variable
    const char *name;
    // Should a prefix be added
    int prefixed;
} am_envattr_conf_t;

extern const command_rec auth_mellon_commands[];

typedef struct am_error_map_t {
    int lasso_error;
    int http_error;
} am_error_map_t;

extern const am_error_map_t auth_mellon_errormap[];

/* When using a value from a directory configuration structure, a special value is used
 * to state "inherit" from parent, when reading a value and the value is still inherit from, it
 * means that no value has ever been set for this directive, in this case, we use the default
 * value.
 *
 * This macro expects that if your variable is called "name" there is a static const variable named
 * "default_name" which holds the default value for this variable.
 */
#define CFG_VALUE(container, name) \
	(container->name == inherit_##name ? default_##name : container->name)

#define CFG_MERGE(add_cfg, base_cfg, name) \
	(add_cfg->name == inherit_##name ? base_cfg->name : add_cfg->name)

/** Default and inherit value for SubjectConfirmationData Address check setting.
 */
static const int default_subject_confirmation_data_address_check = 1;
static const int inherit_subject_confirmation_data_address_check = -1;

/** Default values for seting the cache-control header
 */
static const int default_send_cache_control_header = 1;
static const int inherit_send_cache_control_header = -1;

/* Default and inherit values for MellonPostReplay option. */
static const int default_post_replay = 0;
static const int inherit_post_replay = -1;

/* Whether to send an ECP client a list of IdP's */
static const int default_ecp_send_idplist = 0;
static const int inherit_ecp_send_idplist = -1;

/* Algorithm to use when signing Mellon SAML messages */
static const LassoSignatureMethod default_signature_method =
#if HAVE_DECL_LASSO_SIGNATURE_METHOD_RSA_SHA256
    LASSO_SIGNATURE_METHOD_RSA_SHA256;
#else
    LASSO_SIGNATURE_METHOD_RSA_SHA1;
#endif
static const int inherit_signature_method = -1;

void *auth_mellon_dir_config(apr_pool_t *p, char *d);
void *auth_mellon_dir_merge(apr_pool_t *p, void *base, void *add);
void *auth_mellon_server_config(apr_pool_t *p, server_rec *s);
void *auth_mellon_srv_merge(apr_pool_t *p, void *base, void *add);


const char *am_cookie_get(request_rec *r);
void am_cookie_set(request_rec *r, const char *id);
void am_cookie_delete(request_rec *r);
const char *am_cookie_token(request_rec *r);


apr_status_t
am_get_socache_provider(apr_pool_t *pool, const char *name,
                        ap_socache_provider_t **socache_provider_out,
                        const char **errmsg_out);

void am_cache_update_expires(request_rec *r, am_cache_entry_t *t, apr_time_t expires);
void am_cache_update_idle_timeout(request_rec *r, am_cache_entry_t *t, int session_idle_timeout);

apr_status_t
am_socache_init(apr_pool_t *pool, apr_pool_t *tmp_pool, server_rec *s);


/*--------------------------------- typedefs ---------------------------------*/
/*--------------------------------- defines ----------------------------------*/
/*---------------------------- auth_mellon_cache -----------------------------*/

apr_status_t
am_cache_delete_session_entries(request_rec *r,
                                const char *session_id,
                                LassoSaml2NameID *name_id,
                                LassoSaml2NameID *issuer);

apr_status_t
am_cache_store_session_entries(request_rec *r,
                               const char *session_id,
                               LassoSaml2NameID *name_id,
                               LassoSaml2NameID *issuer,
                               apr_time_t expiration,
                               const char *session_xml);



am_session_state_t *
am_cache_load_session_by_session_id(request_rec *r, const char *session_id);

am_session_state_t *
am_cache_load_session_by_name_id(request_rec *r,
                                 LassoSaml2NameID *name_id,
                                 LassoSaml2NameID *issuer);

#ifdef ENABLE_DIAGNOSTICS

apr_status_t
am_cache_store_diag_dir(request_rec *r, const char *diag_dir, const char *data,
                        apr_time_t expiration);

const char *
am_cache_load_diag_dir(request_rec *r, const char *diag_dir);

#endif
/*---------------------------- auth_mellon_config ----------------------------*/
/*---------------------------- auth_mellon_cookie ----------------------------*/
/*-------------------------- auth_mellon_diagnostics -------------------------*/
/*--------------------------- auth_mellon_handler ----------------------------*/
/*-------------------------- auth_mellon_httpclient --------------------------*/
/*---------------------------- auth_mellon_session ---------------------------*/

apr_status_t
am_session_store(request_rec *r, am_session_state_t *session);

void am_session_update_expires(request_rec *r, am_session_state_t *session,
                               apr_time_t expires);

void
am_session_set_expriation_from_assertion(request_rec *r,
                                         am_session_state_t *session,
                                         LassoSaml2Assertion *assertion);

int
am_session_add_attributes_from_assertion(am_session_state_t *session,
                                         request_rec *r,
                                         LassoSaml2NameID *name_id,
                                         LassoSaml2NameID *issuer,
                                         LassoSaml2Assertion *assertion);

/*----------------------------- auth_mellon_util -----------------------------*/

apr_time_t am_parse_timestamp(request_rec *r, const char *timestamp);

am_session_state_t *
am_session_state_new(request_rec *r);

am_session_state_t *
am_session_state_from_xml(request_rec *r, xmlDocPtr doc);

apr_array_header_t *
am_session_set_env_attr_name(request_rec *r, am_session_state_t *ss,
                                 const char *name);

bool
am_session_env_attr_values_has_value(apr_array_header_t *values,
                                     const char *value);

bool
am_session_env_attr_has_value(request_rec *r, am_session_state_t *ss,
                              const char *name, const char *value);

apr_array_header_t *
am_session_set_env_attr_value(request_rec *r, am_session_state_t *ss,
                              const char *name, const char *value);

apr_array_header_t *
am_session_get_env_attr_values(request_rec *r, am_session_state_t *ss,
                               const char *name);

const char *
am_session_get_first_env_attr_value(request_rec *r, am_session_state_t *ss,
                                    const char *name);

void am_session_export_env(request_rec *r, am_session_state_t *ss);

char *
am_str_join(apr_pool_t *pool, apr_array_header_t *strings,
            const char *separator);

xmlNodePtr
am_xml_get_first_child(xmlNodePtr node, const char *name, const char *ns_href);

char *
am_time_t_to_8601(apr_pool_t *pool, apr_time_t t);

const char *
am_mapped_env_attr_name(request_rec *r, const char *attr_name,
                        const char **prefixed_out);

xmlDocPtr am_get_xml_doc_from_string(request_rec *r, const char *xml_text);

char *am_xml_doc_to_string(request_rec *r, xmlDocPtr doc, int format);

char* am_xmlnode_to_string(request_rec *r, xmlNode *node, int format);

const char *
am_lasso_node_to_string(request_rec *r, LassoNode *node);

apr_status_t
am_normalize_lasso_name_id(request_rec *r, LassoSaml2NameID *name_id,
                           const char *default_name_qualifier);

const char *
am_lasso_name_id_string(request_rec *r, LassoSaml2NameID *name_id);

am_session_state_t *am_get_request_session(request_rec *r);
am_session_state_t *am_session_get_session_by_name_id(request_rec *r,
                                                      LassoSaml2NameID *name_id,
                                                      LassoSaml2NameID *issuer);
am_session_state_t *am_get_request_session_by_assertion_id(request_rec *r,
                                                        LassoSaml2Assertion *assertion);
am_session_state_t *am_new_request_session(request_rec *r);
void am_release_request_session(request_rec *r, am_session_state_t **session_var);
void am_session_delete(request_rec *r, am_session_state_t *session);


char *am_reconstruct_url(request_rec *r);
int am_validate_redirect_url(request_rec *r, const char *url);
int am_check_permissions(request_rec *r, am_session_state_t *session);
void am_set_cache_control_headers(request_rec *r);
int am_read_post_data(request_rec *r, char **data, apr_size_t *length);
char *am_extract_query_parameter(apr_pool_t *pool,
                                 const char *query_string,
                                 const char *name);
char *am_urlencode(apr_pool_t *pool, const char *str);
int am_urldecode(char *data);
int am_check_url(request_rec *r, const char *url);
char *am_generate_id(request_rec *r);
am_file_data_t *am_file_data_new(apr_pool_t *pool, const char *path);
am_file_data_t *am_file_data_copy(apr_pool_t *pool,
                                  am_file_data_t *src_file_data);
apr_status_t am_file_read(am_file_data_t *file_data);
apr_status_t am_file_stat(am_file_data_t *file_data);
char *am_get_endpoint_url(request_rec *r);
int am_postdir_cleanup(request_rec *s);
char *am_htmlencode(request_rec *r, const char *str);
int am_save_post(request_rec *r, const char **relay_state);
const char *am_filepath_dirname(apr_pool_t *p, const char *path);
const char *am_strip_cr(request_rec *r, const char *str);
const char *am_add_cr(request_rec *r, const char *str);
const char *am_xstrtok(request_rec *r, const char *str, 
                       const char *sep, char **last);
void am_strip_blank(const char **s);
const char *am_get_header_attr(request_rec *r, const char *h,
                               const char *v, const char *a);
int am_has_header(request_rec *r, const char *h, const char *v);
const char *am_get_mime_header(request_rec *r, const char *m, const char *h);
const char *am_get_mime_body(request_rec *r, const char *mime);
char *am_get_service_url(request_rec *r, 
                         LassoProfile *profile, char *service_name);
bool am_parse_paos_header(request_rec *r, const char *header, ECPServiceOptions *options_return);
bool am_header_has_media_type(request_rec *r, const char *header,
                              const char *media_type);
const char *am_get_config_langstring(apr_hash_t *h, const char *lang);
int am_get_boolean_query_parameter(request_rec *r, const char *name,
                                   int *return_value, int default_value);
char *am_get_assertion_consumer_service_by_binding(LassoProvider *provider, const char *binding);

#ifdef HAVE_ECP
char *am_ecp_service_options_str(apr_pool_t *pool, ECPServiceOptions options);
bool am_is_paos_request(request_rec *r, int *error_code);
#endif /* HAVE_ECP */

char *
am_saml_response_status_str(request_rec *r, LassoNode *node);

const char *
am_sha256_sum(request_rec *r,
              const unsigned char *data, apr_size_t data_len);

LassoSamlp2RequestAbstract *
am_get_request_abstract(request_rec *r, LassoProfile *profile);

LassoSamlp2StatusResponse *
am_get_status_response(request_rec *r, LassoProfile *profile);

int am_auth_mellon_user(request_rec *r);
int am_check_uid(request_rec *r);
int am_handler(request_rec *r);


int am_httpclient_get(request_rec *r, const char *uri, 
                      void **buffer, apr_size_t *size, 
                      int timeout, long *status);
int am_httpclient_post(request_rec *r, const char *uri,
                       const void *post_data, apr_size_t post_length,
                       const char *content_type,
                       void **buffer, apr_size_t *size);
int am_httpclient_post_str(request_rec *r, const char *uri,
                           const char *post_data,
                           const char *content_type,
                           void **buffer, apr_size_t *size);


/* socache */
#define SOCACHE_ID "mellon-socache"

extern module AP_MODULE_DECLARE_DATA auth_mellon_module;

#ifdef ENABLE_DIAGNOSTICS

#if AP_SERVER_MAJORVERSION_NUMBER < 2 || \
    (AP_SERVER_MAJORVERSION_NUMBER == 2 && AP_SERVER_MINORVERSION_NUMBER < 4)
#error "Diagnostics requires Apache version 2.4 or newer."
#endif

typedef struct {
    bool req_headers_written;
} am_diag_request_data;

const char *
am_diag_cond_str(request_rec *r, const am_cond_t *cond);

int
am_diag_finalize_request(request_rec *r);

const char *
am_diag_lasso_http_method_str(LassoHttpMethod http_method);

void
am_diag_log_session_state(request_rec *r, int level, am_session_state_t *ss,
                          const char *fmt, ...);

void
am_diag_log_file_data(request_rec *r, int level, am_file_data_t *file_data,
                      const char *fmt, ...)
    __attribute__((format(printf,4,5)));

int
am_diag_log_init(apr_pool_t *pc, apr_pool_t *p, apr_pool_t *pt, server_rec *s);

void
am_diag_log_lasso_node(request_rec *r, int level, LassoNode *node,
                       const char *fmt, ...)
    __attribute__((format(printf,4,5)));

void
am_diag_log_saml_status_response(request_rec *r, int level, LassoNode *node,
                                 const char *fmt, ...)
    __attribute__((format(printf,4,5)));

void
am_diag_log_profile(request_rec *r, int level, LassoProfile *profile,
                    const char *fmt, ...)
    __attribute__((format(printf,4,5)));

void
am_diag_printf(request_rec *r, const char *fmt, ...)
    __attribute__((format(printf,2,3)));

void
am_diag_rerror(const char *file, int line, int module_index,
               int level, apr_status_t status,
               request_rec *r, const char *fmt, ...);

/* Define AM_LOG_RERROR log to both the Apache log and diagnostics log */
#define AM_LOG_RERROR(...) AM_LOG_RERROR__(__VA_ARGS__)
/* need additional step to expand macros */
#define AM_LOG_RERROR__(file, line, mi, level, status, r, ...)          \
{                                                                       \
    ap_log_rerror(file, line, mi, level, status, r, __VA_ARGS__);       \
    am_diag_rerror(file, line, mi, level, status, r, __VA_ARGS__);      \
}

#else  /* ENABLE_DIAGNOSTICS */

#define am_diag_log_session_state(...) do {} while(0)
#define am_diag_log_file_data(...) do {} while(0)
#define am_diag_log_lasso_node(...) do {} while(0)
#define am_diag_log_saml_status_response(...) do {} while(0)
#define am_diag_log_profile(...) do {} while(0)
#define am_diag_printf(...) do {} while(0)

/* Define AM_LOG_RERROR log only to the Apache log */
#define AM_LOG_RERROR(...) ap_log_rerror(__VA_ARGS__)

#endif /* ENABLE_DIAGNOSTICS */

#endif /* MOD_AUTH_MELLON_H */
