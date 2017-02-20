/*
 * odb.h
 *
 * This file is a part of php-git2.
 */

#ifndef PHPGIT2_ODB_H
#define PHPGIT2_ODB_H

namespace php_git2
{

    // Explicitly specialize git2_resource destructor for git_odb.

    template<> php_git_odb::~git2_resource()
    {
        git_odb_free(handle);
    }

    // Provide a type which binds a git_odb_writepack along with a callback to a
    // PHP object instance.

    class php_git_odb_writepack:
        private php_zts_base
    {
        friend class php_callback_async_ex<php_git_odb_writepack>;
    public:
        php_git_odb_writepack(TSRMLS_D):
            php_zts_base(TSRMLS_C), writepack(nullptr)
        {
        }

        git_odb_writepack** byval_git2(unsigned argno = std::numeric_limits<unsigned>::max())
        {
            return &writepack;
        }

        void ret(zval* return_value) const
        {
            // Create the PHP object. It will manage the lifetime of the
            // writepack and callback from now on.
            php_git2_make_odb_writepack(return_value,writepack,cb TSRMLS_CC);
        }

    private:
        git_odb_writepack* writepack;
        php_callback_sync* cb;
    };

}

// Functions:

static constexpr auto ZIF_GIT_ODB_NEW = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb**
        >::func<git_odb_new>,
    php_git2::local_pack<
        php_git2::php_resource_ref<php_git2::php_git_odb>
        >,
    1,
    php_git2::sequence<>
    >;

static constexpr auto ZIF_GIT_ODB_FREE = zif_php_git2_function_void<
    php_git2::func_wrapper<
        void,
        git_odb*>::func<git_odb_free>,
    php_git2::local_pack<
        php_git2::php_resource_cleanup<php_git2::php_git_odb>
        >
    >;

static constexpr auto ZIF_GIT_ODB_WRITE_PACK = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_writepack**,
        git_odb*,
        git_transfer_progress_cb,
        void*
        >::func<git_odb_write_pack>,
    php_git2::local_pack<
        php_git2::connector_wrapper<php_git2::php_callback_async_ex<php_git2::php_git_odb_writepack> >,
        php_git2::php_git_odb_writepack,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_callback_handler<php_git2::transfer_progress_callback>
        >,
    2,
    php_git2::sequence<2,0,0>, // pass callback in twice for callback and payload
    php_git2::sequence<1,2,3,0>,
    php_git2::sequence<0,0,0,1>
    >;

// Function Entries:

#define GIT_ODB_FE                                              \
    PHP_GIT2_FE(git_odb_new,ZIF_GIT_ODB_NEW,NULL)               \
    PHP_GIT2_FE(git_odb_free,ZIF_GIT_ODB_FREE,NULL)             \
    PHP_GIT2_FE(git_odb_write_pack,ZIF_GIT_ODB_WRITE_PACK,NULL)

#endif

/*
 * Local Variables:
 * mode:c++
 * indent-tabs-mode:nil
 * tab-width:4
 * End:
 */
