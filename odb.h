/*
 * odb.h
 *
 * Copyright (C) Roger P. Gee
 */

#ifndef PHPGIT2_ODB_H
#define PHPGIT2_ODB_H

namespace php_git2
{
    // Explicitly specialize git2_resource destructor for git_odb and
    // git_odb_object.

    template<> php_git_odb::~git2_resource()
    {
        git_odb_free(handle);
    }

    template<> php_git_odb_object::~git2_resource()
    {
        git_odb_object_free(handle);
    }

    class php_git_odb_writepack;

    // Simplify the typename for an ODB's writepack callback.

    using php_writepack_async_callback = php_callback_async_ex<
        php_git_odb_writepack,
        php_callback_sync_nullable
        >;

    // Provide a type which binds a git_odb_writepack along with a callback to a
    // PHP object instance of type GitODBWritepack.

    class php_git_odb_writepack
    {
        friend php_writepack_async_callback;
    public:
        // Make this object a connector to a php_resource that looks up a
        // php_git_odb resource wrapper.
        using connect_t = php_resource<php_git_odb>;
        using target_t = git_odb_writepack**;

        php_git_odb_writepack(connect_t& conn):
            writepack(nullptr), cb(nullptr), ownerWrapper(conn)
        {
        }

        git_odb_writepack** byval_git2()
        {
            return &writepack;
        }

        void ret(zval* return_value) const
        {
            // Create the PHP object. It will manage the lifetime of the
            // writepack and callback from now on. The writepack's backend is
            // assumably managed by another context and *will not* be managed by
            // the writepack object in this extension module. Also pass in the
            // owner ODB to control lifetime.

            php_git2_make_odb_writepack(
                return_value,
                writepack,
                cb,
                nullptr,
                ownerWrapper.get_object());
        }

    private:
        git_odb_writepack* writepack;
        php_callback_sync* cb;
        connect_t& ownerWrapper;
    };

    // Provide types that extract/bind git_odb_backend instances to a PHP object
    // of type GitODBBackend. One class handles existing objects, and the other
    // class handles creating new ones.

    // Extracts a git_odb_backend from a GitODBBackend PHP value. The
    // GitODBBackend object is updated with a custom backing if none is
    // set. (Thus, this operation is not idempotent.)
    class php_git_odb_backend_byval:
        public php_object<php_odb_backend_object>
    {
    public:
        // Make this object a connector to a php_resource that looks up a
        // php_git_odb resource wrapper.
        using connect_t = php_resource<php_git_odb>;
        using target_t = git_odb_backend*;

        php_git_odb_backend_byval(connect_t& conn):
            ownerWrapper(conn)
        {
        }

        git_odb_backend* byval_git2()
        {
            php_odb_backend_object* object = get_storage();

            // If the object doesn't have a backing, then we create a custom
            // one.
            if (object->kind == php_odb_backend_object::unset) {
                object->create_custom_backend(get_value());
            }

            // If the object is a custom backing then it already is (presumably)
            // set on an ODB backend. (The same is true if the owner is already
            // set.)
            else if (object->kind != php_odb_backend_object::user
                || object->owner != nullptr)
            {
                throw php_git2_exception("The ODB backend is already set on an ODB");
            }

            // Otherwise the object is a user backend and we make it into a
            // conventional backend.
            else {
                object->create_conventional_backend(ownerWrapper.get_object());
            }

            return object->backend;
        }

    private:
        connect_t& ownerWrapper;
    };

    // Returns a git_odb_backend as a GitODBBackend PHP value. The owner ODB is
    // NOT set on the backend object.
    class php_git_odb_backend_byref
    {
    public:
        php_git_odb_backend_byref():
            backend(nullptr)
        {
        }

        git_odb_backend** byval_git2()
        {
            return &backend;
        }

        void ret(zval* return_value)
        {
            // Wrap the git_odb_backend handle in an object zval.
            php_git2_make_odb_backend(return_value,backend,nullptr);
        }

    protected:
        git_odb_backend* get_backend()
        {
            return backend;
        }

    private:
        git_odb_backend* backend;
    };

    // Returns a git_odb_backend as a GitODBBackend PHP value. The owner ODB is
    // set when creating the backend object.
    class php_git_odb_backend_byref_owned:
        public php_git_odb_backend_byref
    {
    public:
        // Make this object a connector to a php_resource that looks up a
        // php_git_odb resource wrapper.
        using connect_t = php_resource<php_git_odb>;
        using target_t = git_odb_backend**;

        php_git_odb_backend_byref_owned(connect_t& conn):
            ownerWrapper(conn)
        {
        }

        void ret(zval* return_value)
        {
            // Wrap the git_odb_backend handle in an object zval. Set the owner
            // to the php_git_odb resource wrapper.

            php_git2_make_odb_backend(return_value,get_backend(),ownerWrapper.get_object());
        }

    private:
        connect_t& ownerWrapper;
    };

    // Provide types that extract/bind git_odb_stream instances to/from a PHP
    // object of type GitODBStream. One class handles existing objects and the
    // other handles creating new ones.

    // Extracts a git_odb_stream from a GitODBStream PHP value.
    class php_git_odb_stream_byval:
        public php_object<php_odb_stream_object>
    {
    public:
        git_odb_stream* byval_git2()
        {
            return get_storage()->stream;
        }
    };

    // Binds a git_odb_stream
    class php_git_odb_stream_byref
    {
    public:
        // Make this object a connector to a php_resource that looks up a
        // php_git_odb resource wrapper.
        using connect_t = php_resource<php_git_odb>;
        using target_t = git_odb_stream**;

        php_git_odb_stream_byref(connect_t& conn):
            stream(nullptr), ownerWrapper(conn)
        {
        }

        git_odb_stream** byval_git2()
        {
            return &stream;
        }

        void ret(zval* return_value)
        {
            // Wrap the git_odb_backend handle in an object zval.
            php_git2_make_odb_stream(return_value,stream,ownerWrapper.get_object());
        }

    private:
        git_odb_stream* stream;
        connect_t& ownerWrapper;
    };

    // Provide a rethandler for git_odb_object_data().
    class php_git_odb_object_data_rethandler
    {
        using PackType = local_pack<php_resource<php_git_odb_object> >;
    public:
        bool ret(const void* retval,zval* return_value,PackType& pack)
        {
            if (retval != nullptr) {
                // Make a binary string for the return value. The length is
                // obtained from the odb_object attached to the local_pack.
                size_t length;
                length = git_odb_object_size(pack.get<0>().get_object()->get_handle());
                RETVAL_STRINGL((const char*)retval,length);
            }
            else {
                RETVAL_NULL();
            }

            return true;
        }
    };

    // Provide a type for handling the git_odb_expand_id array for
    // git_odb_expand_ids().

    class php_git_odb_expand_id_array:
        public php_array_base
    {
    public:
        php_git_odb_expand_id_array():
            ids(nullptr)
        {
        }

        ~php_git_odb_expand_id_array()
        {
            if (ids != nullptr) {
                // Write results over existing array value.
                zval* zvp;
                uint32_t i;
                HashPosition pos;
                HashTable* ht = Z_ARRVAL(value);

                i = 0;
                for (zend_hash_internal_pointer_reset_ex(ht,&pos);
                     (zvp = zend_hash_get_current_data_ex(ht,&pos)) != nullptr;
                     zend_hash_move_forward_ex(ht,&pos))
                {
                    zval zwrite;
                    git_odb_expand_id* expand = ids + i;

                    if (expand->length == 0) {
                        ZVAL_BOOL(&zwrite,0);
                    }
                    else {
                        zval zid;
                        convert_oid(&zid,&expand->id);

                        array_init(&zwrite);
                        add_assoc_zval_ex(&zwrite,"id",sizeof("id")-1,&zid);
                        add_assoc_long_ex(
                            &zwrite,
                            "type",
                            sizeof("type")-1,
                            static_cast<zend_long>(expand->type)
                            );
                    }

                    zend_hash_index_update(ht,pos,&zwrite);
                    i += 1;
                }

                efree(ids);
            }
        }

        git_odb_expand_id* byval_git2()
        {
            zval* zvp;
            uint32_t i;
            HashPosition pos;
            HashTable* ht = Z_ARRVAL(value);
            uint32_t n = zend_hash_num_elements(ht);

            if (n == 0) {
                return &dummy;
            }

            ids = reinterpret_cast<git_odb_expand_id*> (
                emalloc(sizeof(git_odb_expand_id) * n)
                );

            i = 0;
            for (zend_hash_internal_pointer_reset_ex(ht,&pos);
                 (zvp = zend_hash_get_current_data_ex(ht,&pos)) != nullptr;
                 zend_hash_move_forward_ex(ht,&pos))
            {
                git_odb_expand_id* expand = ids + i;
                memset(expand,0,sizeof(git_odb_expand_id));

                if (Z_TYPE_P(zvp) == IS_ARRAY) {
                    array_wrapper wrapper(zvp);

                    if (wrapper.query("id")) {
                        const char* id = wrapper.get_string();
                        size_t len = wrapper.get_string_length();

                        convert_oid_fromstr(&expand->id,id,len);
                        // Note: the length is the number of hex characters.
                        expand->length = static_cast<unsigned short>(len);
                    }

                    if (wrapper.query("type")) {
                        zend_long type = wrapper.get_long();

                        expand->type = static_cast<git_object_t>(type);
                    }
                }
                else {
                    zval ztmp;
                    zval* zstrp;

                    if (Z_TYPE_P(zvp) == IS_STRING) {
                        zstrp = zvp;
                    }
                    else {
                        ZVAL_DUP(&ztmp,zvp);
                        convert_to_string(&ztmp);
                        zstrp = &ztmp;
                    }

                    convert_oid_fromstr(&expand->id,Z_STRVAL_P(zstrp),Z_STRLEN_P(zstrp));
                    expand->length = Z_STRLEN_P(zstrp);
                }

                i += 1;
            }

            return ids;
        }

    private:
        git_odb_expand_id dummy;
        git_odb_expand_id* ids;
    };

} // namespace php_git2

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

static constexpr auto ZIF_GIT_ODB_FREE = zif_php_git2_function_free<
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
        php_git2::php_callback_handler_nullable_async<
            php_git2::transfer_progress_callback,
            php_git2::php_writepack_async_callback
            >,
        php_git2::connector_wrapper<php_git2::php_writepack_async_callback>,
        php_git2::connector_wrapper<php_git2::php_git_odb_writepack>,
        php_git2::php_resource<php_git2::php_git_odb>
        >,
    3,
    php_git2::sequence<3,1,1>, // pass callback in twice for callback and payload
    php_git2::sequence<2,3,0,1>
    >;

static constexpr auto ZIF_GIT_ODB_OPEN = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb**,
        const char*
        >::func<git_odb_open>,
    php_git2::local_pack<
        php_git2::php_resource_ref<php_git2::php_git_odb>,
        php_git2::php_string
        >,
    1,
    php_git2::sequence<1>,
    php_git2::sequence<0,1>
    >;

static constexpr auto ZIF_GIT_ODB_WRITE = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_oid*,
        git_odb*,
        const void*,
        size_t,
        git_object_t
        >::func<git_odb_write>,
    php_git2::local_pack<
        php_git2::php_git_oid,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::connector_wrapper<
            php_git2::php_string_length_connector<size_t> >,
        php_git2::php_string,
        php_git2::php_long_cast<git_object_t>
        >,
    1,
    php_git2::sequence<1,3,4>,
    php_git2::sequence<0,1,3,2,4>
    >;

static constexpr auto ZIF_GIT_ODB_READ = zif_php_git2_function_setdeps<
    php_git2::func_wrapper<
        int,
        git_odb_object**,
        git_odb*,
        const git_oid*
        >::func<git_odb_read>,
    php_git2::local_pack<
        php_git2::php_resource_ref<php_git2::php_git_odb_object>,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_git_oid_fromstr
        >,
    php_git2::sequence<0,1>,
    1,
    php_git2::sequence<1,2>,
    php_git2::sequence<0,1,2>
    >;

static constexpr auto ZIF_GIT_ODB_READ_HEADER = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        size_t*,
        git_object_t*,
        git_odb*,
        const git_oid*
        >::func<git_odb_read_header>,
    php_git2::local_pack<
        php_git2::php_long_ref<size_t>,
        php_git2::php_long_out<git_object_t>,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_git_oid_fromstr
        >,
    1,
    php_git2::sequence<1,2,3>,
    php_git2::sequence<0,1,2,3>
    >;
ZEND_BEGIN_ARG_INFO_EX(git_odb_read_header_arginfo,0,0,3)
    ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO()

static constexpr auto ZIF_GIT_ODB_READ_PREFIX = zif_php_git2_function_setdeps<
    php_git2::func_wrapper<
        int,
        git_odb_object**,
        git_odb*,
        const git_oid*,
        size_t
        >::func<git_odb_read_prefix>,
    php_git2::local_pack<
        php_git2::php_resource_ref<php_git2::php_git_odb_object>,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::connector_wrapper<
            php_git2::php_string_length_connector<size_t,php_git2::php_git_oid_fromstr>
            >,
        php_git2::php_git_oid_fromstr
        >,
    php_git2::sequence<0,1>,
    1,
    php_git2::sequence<1,3>,
    php_git2::sequence<0,1,3,2>
    >;

static constexpr auto ZIF_GIT_ODB_OBJECT_FREE = zif_php_git2_function_free<
    php_git2::local_pack<
        php_git2::php_resource_cleanup<php_git2::php_git_odb_object>
        >
    >;

static constexpr auto ZIF_GIT_ODB_OBJECT_DATA = zif_php_git2_function_rethandler<
    php_git2::func_wrapper<
        const void*,
        git_odb_object*
        >::func<git_odb_object_data>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb_object>
        >,
    php_git2::php_git_odb_object_data_rethandler
    >;

static constexpr auto ZIF_GIT_ODB_OBJECT_SIZE = zif_php_git2_function<
    php_git2::func_wrapper<
        size_t,
        git_odb_object*
        >::func<git_odb_object_size>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb_object>
        >,
    0
    >;

static constexpr auto ZIF_GIT_ODB_OBJECT_ID = zif_php_git2_function<
    php_git2::func_wrapper<
        const git_oid*,
        git_odb_object*
        >::func<git_odb_object_id>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb_object>
        >,
    0
    >;

static constexpr auto ZIF_GIT_ODB_OBJECT_TYPE = zif_php_git2_function<
    php_git2::func_wrapper<
        git_object_t,
        git_odb_object*
        >::func<git_odb_object_type>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb_object>
        >,
    0
    >;

static constexpr auto ZIF_GIT_ODB_OBJECT_DUP = zif_php_git2_function_setdeps<
    php_git2::func_wrapper<
        int,
        git_odb_object**,
        git_odb_object*
        >::func<git_odb_object_dup>,
    php_git2::local_pack<
        php_git2::php_resource_ref<php_git2::php_git_odb_object>,
        php_git2::php_resource<php_git2::php_git_odb_object>
        >,
    php_git2::sequence<0,1>,
    1,
    php_git2::sequence<1>,
    php_git2::sequence<0,1>
    >;

static constexpr auto ZIF_GIT_ODB_BACKEND_PACK = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_backend**,
        const char*
        >::func<git_odb_backend_pack>,
    php_git2::local_pack<
        php_git2::php_git_odb_backend_byref,
        php_git2::php_string
        >,
    1,
    php_git2::sequence<1>,
    php_git2::sequence<0,1>
    >;

static constexpr auto ZIF_GIT_ODB_BACKEND_LOOSE = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_backend**,
        const char*,
        int,
        int,
        unsigned int,
        unsigned int
        >::func<git_odb_backend_loose>,
    php_git2::local_pack<
        php_git2::php_git_odb_backend_byref,
        php_git2::php_string,
        php_git2::php_long,
        php_git2::php_bool,
        php_git2::php_long,
        php_git2::php_long
        >,
    1,
    php_git2::sequence<1,2,3,4,5>,
    php_git2::sequence<0,1,2,3,4,5>
    >;

static constexpr auto ZIF_GIT_ODB_BACKEND_ONE_PACK = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_backend**,
        const char*
        >::func<git_odb_backend_one_pack>,
    php_git2::local_pack<
        php_git2::php_git_odb_backend_byref,
        php_git2::php_string
        >,
    1,
    php_git2::sequence<1>,
    php_git2::sequence<0,1>
    >;

static constexpr auto ZIF_GIT_ODB_OPEN_RSTREAM = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_stream**,
        size_t*,
        git_object_t*,
        git_odb*,
        const git_oid*>::func<git_odb_open_rstream>,
    php_git2::local_pack<
        php_git2::php_long_out<size_t>,
        php_git2::php_long_out<git_object_t>,
        php_git2::connector_wrapper<php_git2::php_git_odb_stream_byref>,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_git_oid_fromstr
        >,
    3,
    php_git2::sequence<0,1,3,4>,
    php_git2::sequence<2,0,1,3,4>
    >;
ZEND_BEGIN_ARG_INFO_EX(git_odb_open_rstream_arginfo,0,0,4)
    ZEND_ARG_PASS_INFO(1)
    ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO()

static constexpr auto ZIF_GIT_ODB_OPEN_WSTREAM = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_stream**,
        git_odb*,
        git_object_size_t,
        git_object_t>::func<git_odb_open_wstream>,
    php_git2::local_pack<
        php_git2::connector_wrapper<php_git2::php_git_odb_stream_byref>,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_long_cast<git_object_size_t>,
        php_git2::php_long_cast<git_object_t>
        >,
    1,
    php_git2::sequence<1,2,3>,
    php_git2::sequence<0,1,2,3>
    >;

static constexpr auto ZIF_GIT_ODB_STREAM_READ = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_stream*,
        char*,
        size_t
        >::func<git_odb_stream_read>,
    php_git2::local_pack<
        php_git2::php_git_odb_stream_byval,
        php_git2::connector_wrapper<php_git2::php_string_buffer_connector>,
        php_git2::php_long_cast<size_t>
        >,
    2,
    php_git2::sequence<0,2>,
    php_git2::sequence<0,1,2>
    >;

static constexpr auto ZIF_GIT_ODB_STREAM_WRITE = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_stream*,
        const char*,
        size_t
        >::func<git_odb_stream_write>,
    php_git2::local_pack<
        php_git2::php_git_odb_stream_byval,
        php_git2::connector_wrapper<php_git2::php_string_length_connector<size_t> >,
        php_git2::php_string
        >,
    -1,
    php_git2::sequence<0,2>,
    php_git2::sequence<0,2,1>
    >;

static constexpr auto ZIF_GIT_ODB_STREAM_FINALIZE_WRITE = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_oid*,
        git_odb_stream*
        >::func<git_odb_stream_finalize_write>,
    php_git2::local_pack<
        php_git2::php_git_odb_stream_byval,
        php_git2::php_git_oid
        >,
    2,
    php_git2::sequence<0>,
    php_git2::sequence<1,0>
    >;

static constexpr auto ZIF_GIT_ODB_ADD_ALTERNATE = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb*,
        git_odb_backend*,
        int
        >::func<git_odb_add_alternate>,
    php_git2::local_pack<
        php_git2::connector_wrapper<php_git2::php_git_odb_backend_byval>,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_long
        >,
    -1,
    php_git2::sequence<1,0,2>,
    php_git2::sequence<1,0,2>
    >;

static constexpr auto ZIF_GIT_ODB_ADD_DISK_ALTERNATE = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb*,
        const char*
        >::func<git_odb_add_disk_alternate>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_string
        >,
    -1
    >;

static constexpr auto ZIF_GIT_ODB_ADD_BACKEND = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb*,
        git_odb_backend*,
        int
        >::func<git_odb_add_backend>,
    php_git2::local_pack<
        php_git2::connector_wrapper<php_git2::php_git_odb_backend_byval>,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_long
        >,
    -1,
    php_git2::sequence<1,0,2>,
    php_git2::sequence<1,0,2>
    >;

static constexpr auto ZIF_GIT_ODB_EXISTS = zif_php_git2_function_rethandler<
    php_git2::func_wrapper<
        int,
        git_odb*,
        const git_oid*
        >::func<git_odb_exists>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_git_oid_fromstr
        >,
    php_git2::php_boolean_rethandler<int>
    >;

static constexpr auto ZIF_GIT_ODB_EXISTS_PREFIX = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_oid*,
        git_odb*,
        const git_oid*,
        size_t
        >::func<git_odb_exists_prefix>,
    php_git2::local_pack<
        php_git2::php_git_oid,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::connector_wrapper<
            php_git2::php_string_length_connector<size_t,php_git2::php_git_oid_fromstr>
            >,
        php_git2::php_git_oid_fromstr
        >,
    1,
    php_git2::sequence<1,3>,
    php_git2::sequence<0,1,3,2>
    >;

static constexpr auto ZIF_GIT_ODB_EXPAND_IDS = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb*,
        git_odb_expand_id*,
        size_t
        >::func<git_odb_expand_ids>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::connector_wrapper<
            php_git2::php_array_length_connector<size_t>
            >,
        php_git2::php_git_odb_expand_id_array
        >,
    -1,
    php_git2::sequence<0,2>,
    php_git2::sequence<0,2,1>
    >;

static constexpr auto ZIF_GIT_ODB_FOREACH = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb*,
        git_odb_foreach_cb,
        void*
        >::func<git_odb_foreach>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_callback_handler<php_git2::odb_foreach_callback>,
        php_git2::php_callback_sync
        >,
    -1,
    php_git2::sequence<0,2,2>, // pass callback in for both callable and payload
    php_git2::sequence<0,1,2>
    >;

static constexpr auto ZIF_GIT_ODB_REFRESH = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb*
        >::func<git_odb_refresh>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb>
        >
    >;

static constexpr auto ZIF_GIT_ODB_GET_BACKEND = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_odb_backend**,
        git_odb*,
        size_t
        >::func<git_odb_get_backend>,
    php_git2::local_pack<
        php_git2::connector_wrapper<php_git2::php_git_odb_backend_byref_owned>,
        php_git2::php_resource<php_git2::php_git_odb>,
        php_git2::php_long_cast<size_t>
        >,
    1,
    php_git2::sequence<1,2>,
    php_git2::sequence<0,1,2>
    >;

static constexpr auto ZIF_GIT_ODB_NUM_BACKENDS = zif_php_git2_function<
    php_git2::func_wrapper<
        size_t,
        git_odb*
        >::func<git_odb_num_backends>,
    php_git2::local_pack<
        php_git2::php_resource<php_git2::php_git_odb>
        >,
    0
    >;

static constexpr auto ZIF_GIT_ODB_HASH = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_oid*,
        const void*,
        size_t,
        git_object_t
        >::func<git_odb_hash>,
    php_git2::local_pack<
        php_git2::php_git_oid,
        php_git2::connector_wrapper<
            php_git2::php_string_length_connector<size_t> >,
        php_git2::php_string,
        php_git2::php_long_cast<git_object_t>
        >,
    1,
    php_git2::sequence<2,3>,
    php_git2::sequence<0,2,1,3>
    >;

static constexpr auto ZIF_GIT_ODB_HASHFILE = zif_php_git2_function<
    php_git2::func_wrapper<
        int,
        git_oid*,
        const char*,
        git_object_t
        >::func<git_odb_hashfile>,
    php_git2::local_pack<
        php_git2::php_git_oid,
        php_git2::php_string,
        php_git2::php_long_cast<git_object_t>
        >,
    1,
    php_git2::sequence<1,2>
    >;

// Function Entries:

#define GIT_ODB_FE                                                      \
    PHP_GIT2_FE(git_odb_new,ZIF_GIT_ODB_NEW,NULL)                       \
    PHP_GIT2_FE(git_odb_free,ZIF_GIT_ODB_FREE,NULL)                     \
    PHP_GIT2_FE(git_odb_write_pack,ZIF_GIT_ODB_WRITE_PACK,NULL)         \
    PHP_GIT2_FE(git_odb_open,ZIF_GIT_ODB_OPEN,NULL)                     \
    PHP_GIT2_FE(git_odb_write,ZIF_GIT_ODB_WRITE,NULL)                   \
    PHP_GIT2_FE(git_odb_read,ZIF_GIT_ODB_READ,NULL)                     \
    PHP_GIT2_FE(git_odb_read_header,ZIF_GIT_ODB_READ_HEADER,git_odb_read_header_arginfo) \
    PHP_GIT2_FE(git_odb_read_prefix,ZIF_GIT_ODB_READ_PREFIX,NULL)       \
    PHP_GIT2_FE(git_odb_object_free,ZIF_GIT_ODB_OBJECT_FREE,NULL)       \
    PHP_GIT2_FE(git_odb_object_data,ZIF_GIT_ODB_OBJECT_DATA,NULL)       \
    PHP_GIT2_FE(git_odb_object_size,ZIF_GIT_ODB_OBJECT_SIZE,NULL)       \
    PHP_GIT2_FE(git_odb_object_id,ZIF_GIT_ODB_OBJECT_ID,NULL)           \
    PHP_GIT2_FE(git_odb_object_type,ZIF_GIT_ODB_OBJECT_TYPE,NULL)       \
    PHP_GIT2_FE(git_odb_object_dup,ZIF_GIT_ODB_OBJECT_DUP,NULL)         \
    PHP_GIT2_FE(git_odb_backend_pack,ZIF_GIT_ODB_BACKEND_PACK,NULL)     \
    PHP_GIT2_FE(git_odb_backend_loose,ZIF_GIT_ODB_BACKEND_LOOSE,NULL)   \
    PHP_GIT2_FE(git_odb_backend_one_pack,ZIF_GIT_ODB_BACKEND_ONE_PACK,NULL) \
    PHP_GIT2_FE(git_odb_open_rstream,ZIF_GIT_ODB_OPEN_RSTREAM,git_odb_open_rstream_arginfo) \
    PHP_GIT2_FE(git_odb_open_wstream,ZIF_GIT_ODB_OPEN_WSTREAM,NULL)     \
    PHP_GIT2_FE(git_odb_stream_read,ZIF_GIT_ODB_STREAM_READ,NULL)       \
    PHP_GIT2_FE(git_odb_stream_write,ZIF_GIT_ODB_STREAM_WRITE,NULL)     \
    PHP_GIT2_FE(git_odb_stream_finalize_write,ZIF_GIT_ODB_STREAM_FINALIZE_WRITE,NULL) \
    PHP_GIT2_FE(git_odb_add_alternate,ZIF_GIT_ODB_ADD_ALTERNATE,NULL)   \
    PHP_GIT2_FE(git_odb_add_disk_alternate,ZIF_GIT_ODB_ADD_DISK_ALTERNATE,NULL) \
    PHP_GIT2_FE(git_odb_add_backend,ZIF_GIT_ODB_ADD_BACKEND,NULL)       \
    PHP_GIT2_FE(git_odb_exists,ZIF_GIT_ODB_EXISTS,NULL)                 \
    PHP_GIT2_FE(git_odb_exists_prefix,ZIF_GIT_ODB_EXISTS_PREFIX,NULL)   \
    PHP_GIT2_FE(git_odb_expand_ids,ZIF_GIT_ODB_EXPAND_IDS,NULL)         \
    PHP_GIT2_FE(git_odb_foreach,ZIF_GIT_ODB_FOREACH,NULL)               \
    PHP_GIT2_FE(git_odb_refresh,ZIF_GIT_ODB_REFRESH,NULL)               \
    PHP_GIT2_FE(git_odb_get_backend,ZIF_GIT_ODB_GET_BACKEND,NULL)       \
    PHP_GIT2_FE(git_odb_num_backends,ZIF_GIT_ODB_NUM_BACKENDS,NULL)     \
    PHP_GIT2_FE(git_odb_hash,ZIF_GIT_ODB_HASH,NULL)                     \
    PHP_GIT2_FE(git_odb_hashfile,ZIF_GIT_ODB_HASHFILE,NULL)

#endif

/*
 * Local Variables:
 * mode:c++
 * indent-tabs-mode:nil
 * tab-width:4
 * End:
 */
