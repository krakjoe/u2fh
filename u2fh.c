/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe                                                      |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/standard/info.h"
#include "php_u2fh.h"

#include <u2f-host.h>

ZEND_BEGIN_MODULE_GLOBALS(u2fh)
	u2fh_devs *devs;
	unsigned count;
ZEND_END_MODULE_GLOBALS(u2fh)

ZEND_DECLARE_MODULE_GLOBALS(u2fh);

#define U2FH_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(u2fh, v)

static void php_u2fh_init_globals(zend_u2fh_globals *u2fh) {
	memset(u2fh, 0, sizeof(zend_u2fh_globals));
}

#define php_u2fh_exception(rc) do { \
	zend_throw_exception_ex(spl_ce_RuntimeException, rc, \
			"%s: %s", \
			u2fh_strerror_name(rc), \
			u2fh_strerror(rc)); \
	return; \
} while(0)

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(u2fh)
{
	ZEND_INIT_MODULE_GLOBALS(u2fh, php_u2fh_init_globals, NULL);

	if (u2fh_global_init(1) != U2FH_OK) {
		return FAILURE;
	}

	REGISTER_NS_LONG_CONSTANT("U2FH", "PING",    0x80 | 0x01, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT("U2FH", "MSG",     0x80 | 0x03, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT("U2FH", "LOCK",    0x80 | 0x04, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT("U2FH", "INIT",    0x80 | 0x06, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT("U2FH", "WINK",    0x80 | 0x08, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT("U2FH", "ERROR",   0x80 | 0x3f, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT("U2FH", "VFIRST",  0x80 | 0x40, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT("U2FH", "VLAST",   0x80 | 0x7f, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(u2fh)
{
	u2fh_global_done();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(u2fh)
{
#if defined(COMPILE_DL_U2FH) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	if (u2fh_devs_init(&U2FH_G(devs)) != U2FH_OK) {
		return FAILURE;
	}
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(u2fh)
{
	if (U2FH_G(devs)) {
		u2fh_devs_done(U2FH_G(devs));
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(u2fh)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "u2f host support", "enabled");
	php_info_print_table_end();
}
/* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(php_u2fh_discover_arginfo, 0, 0, IS_LONG, NULL, 0)
ZEND_END_ARG_INFO()

/* {{{ proto int u2fh_discover(void) */
PHP_FUNCTION(u2fh_discover) 
{
	u2fh_rc rc;

	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

	rc = u2fh_devs_discover(U2FH_G(devs), &U2FH_G(count));

	if (rc != U2FH_OK) { 
		php_u2fh_exception(rc);
	}

	RETURN_LONG(U2FH_G(count));
} /* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(php_u2fh_describe_arginfo, 0, 1, IS_STRING, NULL, 1)
	ZEND_ARG_TYPE_INFO(0, dev, IS_LONG, 0)
ZEND_END_ARG_INFO()

/* {{{ proto string u2fh\describe(int index) */
PHP_FUNCTION(u2fh_describe)
{
	zend_long dev = 0;
	u2fh_rc rc;
	zend_string *desc;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l", &dev) != SUCCESS) {
		return;
	}
	
	desc = zend_string_alloc(255, 0);

	rc = u2fh_get_device_description(U2FH_G(devs), dev, ZSTR_VAL(desc), &ZSTR_LEN(desc));

	if (rc != U2FH_OK) {
		zend_string_release(desc);

		php_u2fh_exception(rc);
	}

	RETVAL_STR(desc);
} /* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(php_u2fh_ping_arginfo, 0, 1, _IS_BOOL, NULL, 0)
	ZEND_ARG_TYPE_INFO(0, dev, IS_LONG, 0)
ZEND_END_ARG_INFO()

/* {{{ proto bool u2fh\ping(int index) */
PHP_FUNCTION(u2fh_ping)
{
	zend_long dev = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l", &dev) != SUCCESS) {
		return;
	}

	RETURN_BOOL(u2fh_is_alive(U2FH_G(devs), (unsigned) dev));
} /* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(php_u2fh_register_arginfo, 0, 1, IS_STRING, NULL, 0)
	ZEND_ARG_TYPE_INFO(0, challenge, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, origin, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, presence, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

/* {{{ proto string u2fh\register(string challenge, string origin [, bool presence = false]) */
PHP_FUNCTION(u2fh_register)
{
	zend_string *challenge = NULL, *origin = NULL, *response = NULL;
	zend_bool presence = 0;
	u2fh_rc rc;
	size_t block = 1024 + 512;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "SS|b", &challenge, &origin, &presence) != SUCCESS) {
		return;
	}

	response = zend_string_alloc(2048, 0);

	rc = u2fh_register2(U2FH_G(devs), 
		ZSTR_VAL(challenge), ZSTR_VAL(origin), 
			ZSTR_VAL(response), &ZSTR_LEN(response), 
				presence ? U2FH_REQUEST_USER_PRESENCE : 0);

	if (rc != U2FH_OK) {
		zend_string_release(response);

		php_u2fh_exception(rc);
	}

	RETVAL_STR(response);
} /* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(php_u2fh_authenticate_arginfo, 0, 1, IS_STRING, NULL, 0)
	ZEND_ARG_TYPE_INFO(0, challenge, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, origin, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, presence, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

/* {{{ proto string u2fh\authenticate(string challenge, string origin [, bool presence = false]) */
PHP_FUNCTION(u2fh_authenticate)
{
	zend_string *challenge = NULL, *origin = NULL, *response = NULL;
	zend_bool presence = 0;
	u2fh_rc rc;
	size_t block = 1024 + 512;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "SS|b", &challenge, &origin, &presence) != SUCCESS) {
		return;
	}

	response = zend_string_alloc(2048, 0);

	rc = u2fh_authenticate2(U2FH_G(devs), 
		ZSTR_VAL(challenge), ZSTR_VAL(origin), 
			ZSTR_VAL(response), &ZSTR_LEN(response), 
				presence ? U2FH_REQUEST_USER_PRESENCE : 0);

	if (rc != U2FH_OK) {
		zend_string_release(response);

		php_u2fh_exception(rc);
	}

	RETVAL_STR(response);
} /* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(php_u2fh_sendrecv_arginfo, 0, 1, IS_STRING, NULL, 3)
	ZEND_ARG_TYPE_INFO(0, dev, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, cmd, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* {{{ proto string u2fh\sendrecv(int dev, int cmd, string data) */
PHP_FUNCTION(u2fh_sendrecv)
{
	zend_long dev = 0;
	zend_long cmd = 0;
	zend_string *data = NULL, *response = NULL;
	u2fh_rc rc;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "llS", &dev, &cmd, &data) != SUCCESS) {
		return;
	}

	response = zend_string_alloc(4096, 0);

	rc = u2fh_sendrecv(U2FH_G(devs), 
		(unsigned) dev, (uint8_t) cmd, 
		ZSTR_VAL(data), (uint16_t) ZSTR_LEN(data), 
		ZSTR_VAL(response), &ZSTR_LEN(response));

	if (rc != U2FH_OK) {
		zend_string_release(response);
		
		php_u2fh_exception(rc);
	}

	RETVAL_STR(response);
} /* }}} */

/* {{{ u2fh_functions[]
 */
const zend_function_entry u2fh_functions[] = {
	ZEND_NS_FENTRY("u2fh", discover,      ZEND_FN(u2fh_discover),      php_u2fh_discover_arginfo, 0)
	ZEND_NS_FENTRY("u2fh", describe,      ZEND_FN(u2fh_describe),      php_u2fh_describe_arginfo, 0)
	ZEND_NS_FENTRY("u2fh", ping,          ZEND_FN(u2fh_ping),          php_u2fh_ping_arginfo, 0)
	ZEND_NS_FENTRY("u2fh", register,     ZEND_FN(u2fh_register),      php_u2fh_register_arginfo, 0)
	ZEND_NS_FENTRY("u2fh", authenticate,  ZEND_FN(u2fh_authenticate),  php_u2fh_authenticate_arginfo, 0)
	ZEND_NS_FENTRY("u2fh", sendrecv,      ZEND_FN(u2fh_sendrecv),      php_u2fh_sendrecv_arginfo, 0)
	PHP_FE_END
};
/* }}} */

/* {{{ u2fh_module_entry
 */
zend_module_entry u2fh_module_entry = {
	STANDARD_MODULE_HEADER,
	"u2fh",
	u2fh_functions,
	PHP_MINIT(u2fh),
	PHP_MSHUTDOWN(u2fh),
	PHP_RINIT(u2fh),	
	PHP_RSHUTDOWN(u2fh),
	PHP_MINFO(u2fh),
	PHP_U2FH_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_U2FH
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(u2fh)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
