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

namespace php_git2
{

    class php_git2_exception:
        public std::exception
    {
    public:
        php_git2_exception(const char* format, ...);

        virtual const char* what() const noexcept
        { return message.c_str(); }
    private:
        std::string message;
    };

} // namespace php_git2

#endif

/*
 * Local Variables:
 * mode:c++
 * indent-tabs-mode:nil
 * tab-width:4
 * End:
 */
