/*
 * php-git2.h
 *
 * This file is a part of php-git2.
 */

#ifndef PHPGIT2_GIT2_H
#define PHPGIT2_GIT2_H

// Include PHP headers.
extern "C" {
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <php.h>
#include <zend.h>
#include <zend_exceptions.h>
#include <zend_interfaces.h>
#include <ext/standard/php_smart_str.h>
#include <ext/spl/spl_exceptions.h>
#include <ext/standard/php_var.h>
#include <ext/standard/php_string.h>
#include <ext/standard/php_incomplete_class.h>
#include <ext/standard/info.h>
#include <ext/standard/php_array.h>
}

#define PHP_GIT2_EXTNAME "git2"
#define PHP_GIT2_EXTVER  "0.0.0"

// Include the libgit2 headers. They should be installed on the system already.
extern "C" {
#include <git2.h>
}

// Include any C/C++ standard libraries.
#include <string>
#include <exception>
#include <cstring>

// Module globals
ZEND_BEGIN_MODULE_GLOBALS(git2)

ZEND_END_MODULE_GLOBALS(git2)
ZEND_EXTERN_MODULE_GLOBALS(git2)

// Provide macros for handling functionality for threaded PHP builds.

#ifdef ZTS
#define ZTS_CONSTRUCTOR(type) type(TSRMLS_D) {}
#define ZTS_CONSTRUCTOR_WITH_BASE(type,base) type(TSRMLS_D): base(TSRMLS_C) {}
#define ZTS_MEMBER_C(obj) obj.TSRMLS_C
#define ZTS_MEMBER_CC(obj) , obj.TSRMLS_C
#define ZTS_MEMBER_PC(obj) obj->TSRMLS_C
#define ZTS_MEMBER_PCC(obj) , obj->TSRMLS_C

// We provide a macro that allows us to optionally pass TSRMLS_C to a
// constructor.
#define ZTS_CTOR (TSRMLS_C)

#else

#define ZTS_CONSTRUCTOR(type)
#define ZTS_CONSTRUCTOR_WITH_BASE(type,base)
#define ZTS_MEMBER_C(obj)
#define ZTS_MEMBER_CC(obj)
#define ZTS_MEMBER_PC(obj)
#define ZTS_MEMBER_PCC(obj)

// Make ZTS_CTOR insert nothing. It cannot be "( )" since because C++ resolves
// ambiguity in parsing 'T X()' in favor of a function declaration.
#define ZTS_CTOR

#endif

// Simplify constant registration.

#define PHP_GIT2_CONSTANT(name) \
    REGISTER_LONG_CONSTANT(#name,name,CONST_CS|CONST_PERSISTENT)

// Simplify common array lookup operations for 'class php_value' derivatives.

#define GIT2_ARRAY_LOOKUP_VARIABLES(value)                \
    zval** zvp;                                           \
    zval* zv;                                             \
    int srclen = 0;                                       \
    const char* src = nullptr;                            \
    HashTable* ht = Z_ARRVAL_P(value);                    \
    (void)src;                                            \
    (void)srclen;

#define GIT2_ARRAY_EXISTS(name)                                         \
    (zend_hash_find(ht,#name,sizeof(#name),(void**)&zvp) != FAILURE)

#define GIT2_ARRAY_LOOKUP_SUBOBJECT_BYREF(srcobj,name,obj)  \
    if (GIT2_ARRAY_EXISTS(name)) {                          \
        zv = *zvp;                                          \
        *srcobj.byref_php(argno) = zv;                      \
        obj.name = srcobj.byval_git2(argno);                \
    }

#define GIT2_ARRAY_LOOKUP_SUBOBJECT_BYVAL(srcobj,name,obj)  \
    if (GIT2_ARRAY_EXISTS(name)) {                          \
        zv = *zvp;                                          \
        *srcobj.byref_php(argno) = zv;                      \
        obj.name = *srcobj.byval_git2(argno);               \
    }

#define GIT2_ARRAY_LOOKUP_LONG(name,obj)                                \
    if (GIT2_ARRAY_EXISTS(name)) {                                      \
        zv = *zvp;                                                      \
        convert_to_long(zv);                                            \
        obj.name = (decltype(obj.name)) Z_LVAL_P(zv);                   \
    }

#define GIT2_ARRAY_LOOKUP_STRING(name,obj)                              \
    if (GIT2_ARRAY_EXISTS(name)) {                          \
        zv = *zvp;                                                      \
        convert_to_string(zv);                                          \
        obj.name = Z_STRVAL_P(zv);                                      \
    }

#define GIT2_ARRAY_LOOKUP_STRING_NULLABLE(name,obj)                     \
    if (GIT2_ARRAY_EXISTS(name)) {                                      \
        zv = *zvp;                                                      \
        if (Z_TYPE_P(value) == IS_NULL) {                               \
            obj.name = nullptr;                                         \
        }                                                               \
        else {                                                          \
            convert_to_string(zv);                                      \
            obj.name = Z_STRVAL_P(zv);                                  \
        }                                                               \
    }

#define GIT2_ARRAY_LOOKUP_OID(name,obj)                                 \
    if (GIT2_ARRAY_EXISTS(name)) {                                      \
        zv = *zvp;                                                      \
        convert_to_string(zv);                                          \
        src = Z_STRVAL_P(zv);                                           \
        srclen = Z_STRLEN_P(zv);                                        \
        convert_oid_fromstr(&obj.name,src,srclen);                      \
    }

#define GIT2_ARRAY_LOOKUP_RESOURCE(type,name,obj)                       \
    if (GIT2_ARRAY_EXISTS(name)) {                                      \
        php_resource<type> wrapper;                                     \
        type resource;                                                  \
        zv = *zvp;                                                      \
        if (Z_TYPE_P(zv) != IS_NULL) {                                  \
            if (Z_TYPE_P(zv) != IS_RESOURCE) {                          \
                error_custom("expected resource in property " #name,argno); \
            }                                                           \
            *wrapper.byref_php(argno) = zv;                             \
            resource = wrapper.get_object(argno);                       \
            obj.name = resource.get_handle();                           \
        }                                                               \
    }

#define UNUSED(var) ((void)var)

namespace php_git2
{

    // Provide a base exception type for identifying exceptions we care to
    // handle.

    class php_git2_exception_base:
        public std::exception
    {
    public:
        php_git2_exception_base():
            code(0) {}

        // The error code to report to userspace.
        int code;
    };

    // Provide an exception type to be thrown by git2 function wrappers when
    // they fail. This will allow userspace to handle these errors.

    class php_git2_exception:
        public php_git2_exception_base
    {
    public:
        php_git2_exception(const char* format, ...);

        virtual const char* what() const noexcept
        { return message.c_str(); }
    private:
        std::string message;
    };

    // Provide an exception type to be thrown when a PHP exception is to be
    // propagated through.

    class php_git2_propagated_exception:
        public php_git2_exception_base
    {
    public:
        // Return NULL pointer to indicate that the exception propagates
        // through.
        virtual const char* what() const noexcept
        { return nullptr; }
    };

    // Provide a function to handle a generic libgit2 error. It is generic for
    // any possible return type. However we specialize it for int to handle all
    // cases. We want to preserve the numeric return code so we can expose it to
    // userspace.

    template<typename T>
    void git_error(T t)
    {
        // Default to GIT_ERROR if the return_type is not 'int'.

        git_error<int>(GIT_ERROR);
    }

    template<>
    void git_error(int code);

    // Misc. helpers

    void convert_oid_fromstr(git_oid* dest,const char* src,int srclen);
    void php_git2_register_constants(int module_number TSRMLS_DC);

} // namespace php_git2

#endif

/*
 * Local Variables:
 * mode:c++
 * indent-tabs-mode:nil
 * tab-width:4
 * End:
 */
