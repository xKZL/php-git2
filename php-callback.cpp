/*
 * php-callback.cpp
 *
 * This file is a part of php-git2.
 *
 * This unit provides the out-of-line implementations of the various callback
 * routines used by php-git2.
 */

#include "php-callback.h"
using namespace php_git2;

// packbuilder_foreach_callback

/*static*/ int
packbuilder_foreach_callback::callback(void* buf,size_t size,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(cb);
#endif

    if (cb != nullptr) {
        // Special Case: if the callback is null but the payload is a stream, we
        // write directly to the stream.
        if (Z_TYPE_P(cb->func) == IS_NULL && cb->data != nullptr && Z_TYPE_P(cb->data) == IS_RESOURCE
            && strcmp(zend_rsrc_list_get_rsrc_type(Z_RESVAL_P(cb->data) TSRMLS_CC),"stream") == 0)
        {
            php_stream* stream = nullptr;
            php_stream_from_zval_no_verify(stream,&cb->data);
            if (stream != nullptr) {
                php_stream_write(stream,(const char*)buf,size);
                return 0;
            }
        }

        // Null callable is a no-op.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        // Otherwise we convert the values to zvals and call PHP userspace.
        long r;
        zval retval;
        zval_array<3> params ZTS_CTOR;
        params.assign<0>(std::forward<const void*>(buf),
            std::forward<size_t>(size),
            std::forward<long>(size),
            std::forward<zval*>(cb->data));
        params.call(cb->func,&retval);
        convert_to_long(&retval);
        r = Z_LVAL(retval);
        zval_dtor(&retval);
        return r;
    }

    return GIT_EUSER;
}

// transfer_progress_callback

/*static*/ int
transfer_progress_callback::callback(const git_transfer_progress* stats,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(cb);
#endif

    if (cb != nullptr) {
        // Avoid setup for null callables.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        // Call the PHP userspace callback. We convert the git_transfer_progress
        // struct to a PHP array.

        long r;
        zval* zstats;
        zval retval;
        zval_array<2> params ZTS_CTOR;

        zstats = params[0];
        params.assign<1>(std::forward<zval*>(cb->data));
        array_init(zstats);
        add_assoc_long_ex(zstats,"total_objects",sizeof("total_objects"),stats->total_objects);
        add_assoc_long_ex(zstats,"indexed_objects",sizeof("indexed_objects"),stats->indexed_objects);
        add_assoc_long_ex(zstats,"received_objects",sizeof("received_objects"),stats->received_objects);
        add_assoc_long_ex(zstats,"local_objects",sizeof("local_objects"),stats->local_objects);
        add_assoc_long_ex(zstats,"total_objects",sizeof("total_objects"),stats->total_objects);
        add_assoc_long_ex(zstats,"indexed_deltas",sizeof("indexed_deltas"),stats->indexed_deltas);
        add_assoc_long_ex(zstats,"received_bytes",sizeof("received_bytes"),stats->received_bytes);

        params.call(cb->func,&retval);
        convert_to_long(&retval);
        r = Z_LVAL(retval);
        zval_dtor(&retval);

        return r;
    }

    return GIT_EUSER;
}

// odb_foreach_callback

/*static*/ int
odb_foreach_callback::callback(const git_oid* oid,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(cb);
#endif

    if (cb != nullptr) {
        // Avoid setup for null callables.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        // Call the PHP userspace callback. We convert the git_oid struct to a
        // PHP string.

        long r;
        zval retval;
        zval_array<2> params ZTS_CTOR;
        char buf[GIT_OID_HEXSZ + 1];

        git_oid_tostr(buf,sizeof(buf),oid);
        ZVAL_STRING(params[0],buf,1);
        params.assign<1>(std::forward<zval*>(cb->data));
        params.call(cb->func,&retval);
        convert_to_long(&retval);
        r = Z_LVAL(retval);
        zval_dtor(&retval);

        return r;
    }

    return GIT_EUSER;
}

// treewalk_callback

/*static*/ int
treewalk_callback::callback(const char* root,const git_tree_entry* entry,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(pc);
#endif

    if (cb != nullptr) {
        // Account for when callable is null and do nothing.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        long r;
        zval retval;
        zval_array<3> params ZTS_CTOR;
        const php_resource_ref<php_git_tree_entry_nofree> res; // it cannot free

        // Convert arguments to PHP values.
        params.assign<0>(std::forward<const char*>(root));
        *res.byval_git2() = entry;
        res.ret(params[1]);
        params.assign<2>(std::forward<zval*>(cb->data));

        // Call the userspace callback.
        params.call(cb->func,&retval);
        convert_to_long(&retval);
        r = Z_LVAL(retval);
        zval_dtor(&retval);

        return r;
    }

    return GIT_EUSER;
}

// commit_parent_callback

/*static*/ const git_oid*
commit_parent_callback::callback(size_t idx,void* payload)
{
    commit_parent_callback::sync_callback* cb
        = reinterpret_cast<commit_parent_callback::sync_callback*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(pc);
#endif

    if (cb != nullptr) {
        // Account for when callable is null and do nothing.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return nullptr;
        }

        zval retval;
        git_oid* oid;
        zval_array<2> params ZTS_CTOR;
        INIT_ZVAL(retval);

        // Convert arguments to PHP values.
        params.assign<0>(std::forward<long>(idx));
        params.assign<1>(std::forward<zval*>(cb->data));

        // Call the userspace callback.
        params.call(cb->func,&retval);

        // Returning null means the callback ends.
        if (Z_TYPE(retval) == IS_NULL) {
            return nullptr;
        }

        // Otherwise a string is returning representing the ID of a commit. We
        // convert this to binary and store the result in the git_oid attached
        // to the callback.
        oid = &cb->oidbuf;
        convert_to_string(&retval);
        convert_oid_fromstr(oid,Z_STRVAL(retval),Z_STRLEN(retval));

        return oid;
    }

    return nullptr;
}

// reference_foreach_callback

int reference_foreach_callback::callback(git_reference* ref,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(pc);
#endif

    if (cb != nullptr) {
        // Account for when callable is null and do nothing.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        long r;
        zval retval;
        zval_array<2> params ZTS_CTOR;
        const php_resource_ref<php_git_reference> res;

        // Convert arguments to PHP values.
        *res.byval_git2() = ref;
        res.ret(params[0]);
        params.assign<1>(std::forward<zval*>(cb->data));

        // Call the userspace callback.
        params.call(cb->func,&retval);
        convert_to_long(&retval);
        r = Z_LVAL(retval);
        zval_dtor(&retval);

        return r;
    }

    return GIT_EUSER;
}

// reference_foreach_name_callback

int reference_foreach_name_callback::callback(const char* name,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(pc);
#endif

    if (cb != nullptr) {
        // Account for when callable is null and do nothing.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        long r;
        zval retval;
        zval_array<2> params ZTS_CTOR;

        // Convert arguments to PHP values.
        params.assign<0>(std::forward<const char*>(name));
        params.assign<1>(std::forward<zval*>(cb->data));

        // Call the userspace callback.
        params.call(cb->func,&retval);
        convert_to_long(&retval);
        r = Z_LVAL(retval);
        zval_dtor(&retval);

        return r;
    }

    return GIT_EUSER;
}

// packbuilder_progress_callback

int packbuilder_progress_callback::callback(int stage,uint32_t current,uint32_t total,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(cb);
#endif

    if (cb != nullptr) {
        // Avoid setup for null callables.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        long r;
        zval retval;
        zval_array<4> params ZTS_CTOR;

        // Convert arguments to PHP values.
        params.assign<0>(std::forward<long>((long)stage));
        params.assign<1>(std::forward<long>((long)current));
        params.assign<2>(std::forward<long>((long)total));
        params.assign<3>(std::forward<zval*>(cb->data));

        // Call the userspace callback.
        params.call(cb->func,&retval);
        convert_to_long(&retval);
        r = Z_LVAL(retval);
        zval_dtor(&retval);

        return r;
    }

    return GIT_EUSER;
}

// config_foreach_callback

int config_foreach_callback::callback(const git_config_entry* entry,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(cb);
#endif

    if (cb != nullptr) {
        // Avoid setup for null callables.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        // NOTE: if this callback returns non-zero the iteration stops. The
        // default return value of NULL will be converted to 0 to provide the
        // default behavior of continuing iteration.

        long r;
        zval* zentry;
        zval retval;
        zval_array<2> params ZTS_CTOR;

        // Convert arguments to PHP values.
        zentry = params[0];
        params.assign<1>(std::forward<zval*>(cb->data));
        array_init(zentry);
        add_assoc_string(zentry,"name",(char*)entry->name,1);
        add_assoc_string(zentry,"value",(char*)entry->value,1);
        add_assoc_long(zentry,"level",entry->level);

        // Call the userspace callback.
        params.call(cb->func,&retval);
        convert_to_long(&retval);
        r = Z_LVAL(retval);
        zval_dtor(&retval);

        return r;
    }

    return GIT_EUSER;
}

// tag_foreach_callback

int tag_foreach_callback::callback(const char* name,git_oid* oid,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(cb);
#endif

    if (cb != nullptr) {
        // Avoid setup for null callables.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        int error;
        zval retval;
        char buf[GIT_OID_HEXSZ+1];
        zval_array<3> params ZTS_CTOR;

        git_oid_tostr(buf,sizeof(buf),oid);
        params.assign<0>(
            std::forward<const char*>(name),
            std::forward<const char*>(buf),
            std::forward<zval*>(cb->data));

        params.call(cb->func,&retval);
        convert_to_long(&retval);
        error = (int)Z_LVAL(retval);
        zval_dtor(&retval);

        return error;
    }

    return GIT_EUSER;
}

// repository_create_callback

int repository_create_callback::callback(git_repository** out,const char* path,int bare,void* payload)
{
    php_callback_sync* cb = reinterpret_cast<php_callback_sync*>(payload);
#ifdef ZTS
    TSRMLS_D = ZTS_MEMBER_PC(cb);
#endif

    if (cb != nullptr) {
        // Avoid setup for null callables.
        if (Z_TYPE_P(cb->func) == IS_NULL) {
            return GIT_OK;
        }

        zval retval;
        zval_array<3> params ZTS_CTOR;

        params.assign<0>(
            std::forward<const char*>(path),
            std::forward<bool>(bare),
            std::forward<zval*>(cb->data));

        params.call(cb->func,&retval);

        // Allow userspace to generate failure by returning null or false.
        if (Z_TYPE(retval) == IS_NULL || (Z_TYPE(retval) == IS_BOOL && !Z_BVAL(retval))) {
            return -1;
        }

        // Make sure a git_repository resource was returned.
        if (Z_TYPE(retval) != IS_RESOURCE) {
            php_error(E_WARNING,"repository_create_callback: must return git_repository resource");
            return -1;
        }

        // Extract resource from return value.
        php_git_repository* resource;
        resource = (php_git_repository*)zend_fetch_resource(
            NULL TSRMLS_CC,
            Z_RESVAL(retval),
            php_git_repository::resource_name(),
            NULL,
            1,
            php_git_repository::resource_le());
        if (resource == nullptr) {
            // NOTE: PHP raises error if resource type didn't match.
            return -1;
        }

        // The repository resource *MUST* be owned (i.e. this resource is
        // responsible for freeing the handle). We assume that the caller is
        // going to return the repository as an owned resource (potentially),
        // and at the very least, the caller is responsible for freeing the repo
        // handle we assign here. Therefore, we cannot have a git_repository
        // handle from a non-owned context (since it could be freed in another
        // context at some future point). Furthermore, we must revoke ownership
        // on this resource to avoid a double free.
        if (!resource->is_owned()) {
            php_error(E_WARNING,"repository_create_callback: cannot return non-owner resource");
            return -1;
        }

        resource->revoke_ownership();
        *out = resource->get_handle();

        return 0;
    }

    return GIT_EUSER;
}

/*
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * End:
 */
