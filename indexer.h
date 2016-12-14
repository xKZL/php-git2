/*
 * indexer.h
 *
 * This file is a part of php-git.
 */

#ifndef PHPGIT2_INDEXER_H
#define PHPGIT2_INDEXER_H

namespace php_git2
{

    // Specialize resource type destructor.

    template<> php_git_indexer::~git2_resource()
    {
        git_indexer_free(handle);
    }

    // Provide subclass of git2_resource for indexer. This is necessary so we
    // can store git_transfer_progress and callback instances alongside the
    // opaque git2 handle within the resource.

    class php_git_indexer_with_stats:
        public php_git_indexer
    {
        friend class php_git_indexer_connector;
        friend class php_callback_async<php_git_indexer_with_stats>;
    public:
        php_git_indexer_with_stats()
        {
            memset(&stats,0,sizeof(git_transfer_progress));
        }

        ~php_git_indexer_with_stats()
        {
            // Free callback structure.
            cb->~php_callback_sync();
            efree(cb);
        }
    private:
        git_transfer_progress stats;
        php_callback_sync* cb;
    };

    class php_git_indexer_connector
    {
    public:
        using connect_t = php_resource<php_git_indexer_with_stats>;
        typedef git_transfer_progress* target_t;

        php_git_indexer_connector(connect_t&& obj):
            conn(std::forward<connect_t>(obj))
        {
        }

        target_t byval_git2(unsigned argno)
        {
            return &conn.get_object(argno)->stats;
        }
    private:
        connect_t&& conn;
    };

} // namespace php_git2

// Functions:

static constexpr auto ZIF_GIT_INDEXER_NEW = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_indexer**,
        const char*,
        unsigned int,
        git_odb*,
        git_transfer_progress_cb,
        void*>::func<git_indexer_new>,
    php_git2::local_pack<
        php_git2::connector_wrapper<php_git2::php_callback_async<php_git2::php_git_indexer_with_stats> >,
        php_git2::php_resource_ref<php_git2::php_git_indexer_with_stats>,
        php_git2::php_string,
        php_git2::php_long,
        php_git2::php_resource_null<php_git2::php_git_odb>,
        php_git2::php_callback_handler<php_git2::transfer_progress_callback> >,
    2,
    php_git2::sequence<2,3,4,0,0>, // pass callback in twice
    php_git2::sequence<1,2,3,4,5,0>,
    php_git2::sequence<0,0,1,2,3,4> >;

static constexpr auto ZIF_GIT_INDEXER_HASH = zif_php_git2_function<
    php_git2::func_wrapper<
        const git_oid*,
        const git_indexer*>::func<git_indexer_hash>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_indexer> >,
    0 >;

static constexpr auto ZIF_GIT_INDEXER_APPEND = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_indexer*,
        const void*,
        size_t,
        git_transfer_progress*>::func<git_indexer_append>,
    php_git2::local_pack<
        php_git2::connector_wrapper<php_git2::php_git_indexer_connector>,
        php_git2::php_resource<php_git2::php_git_indexer_with_stats>,
        php_git2::connector_wrapper<php_git2::php_string_length_connector<size_t> >,
        php_git2::php_string>,
    -1,
    php_git2::sequence<1,3>,
    php_git2::sequence<1,3,2,0>,
    php_git2::sequence<0,0,1,0> >;

static constexpr auto ZIF_GIT_INDEXER_COMMIT = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_indexer*,
        git_transfer_progress*>::func<git_indexer_commit>,
    php_git2::local_pack<
        php_git2::connector_wrapper<php_git2::php_git_indexer_connector>,
        php_git2::php_resource<php_git2::php_git_indexer_with_stats> >,
    -1,
    php_git2::sequence<1>,
    php_git2::sequence<1,0>,
    php_git2::sequence<0,0> >;

static constexpr auto ZIF_GIT_INDEXER_FREE = zif_php_git2_function_void<
    php_git2::func_wrapper<
        void,
        git_indexer*>::func<git_indexer_free>,
    php_git2::local_pack<
        php_git2::php_resource_cleanup<php_git2::php_git_indexer> > >;

// Function Entries:

#define GIT_INDEXER_FE                                          \
    PHP_GIT2_FE(git_indexer_new,ZIF_GIT_INDEXER_NEW,NULL)       \
    PHP_GIT2_FE(git_indexer_hash,ZIF_GIT_INDEXER_HASH,NULL)     \
    PHP_GIT2_FE(git_indexer_append,ZIF_GIT_INDEXER_APPEND,NULL) \
    PHP_GIT2_FE(git_indexer_commit,ZIF_GIT_INDEXER_COMMIT,NULL) \
    PHP_GIT2_FE(git_indexer_free,ZIF_GIT_INDEXER_FREE,NULL)

#endif

/*
 * Local Variables:
 * mode:c++
 * indent-tabs-mode:nil
 * tab-width:4
 * End:
 */
