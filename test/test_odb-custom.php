<?php

namespace Git2Test\ODBCustom;

use DateTime;
use PHPSerializedODB;
use PHPSerializedODB_WithStream;

require_once 'test_base.php';
require_once 'PHPSerializedODB.php';

function test_session_backend() {
    // Create custom backend. NOTE: This backend does not implement its own
    // writestream() method so we get a fake wstream with for_each().
    $backend = new PHPSerializedODB('odbcustom.test_session_backend');
    $repo = git_repository_new();
    $odb = git_odb_new();

    git_odb_add_backend($odb,$backend,1);
    git_repository_set_odb($repo,$odb);

    // Create a blob in the repository.
    $data =<<<EOF
PHP is a general purpose programming language, especially well-suited for Web
development. It can be extended through writing extension modules that are either
compiled directly into the engine or loaded dynamically at runtime.
EOF;
    $oid = git_blob_create_frombuffer($repo,$data);
    echo "Created blob with oid=$oid\n";

    // Stream the objects from the test repository into our new repository.
    $srcrepo = git_repository_open_bare(testbed_get_repo_path());
    $srcodb = git_repository_odb($srcrepo);

    $lambda = function($oid,$store) {
        list($src,$dst) = $store;

        $obj = git_odb_read($src,$oid);
        $data = git_odb_object_data($obj);
        $type = git_odb_object_type($obj);

        $stream = git_odb_open_wstream($dst,strlen($data),$type);
        testbed_do_once('Git2Test.ODBCustom.transfer_object1',function() use($stream) {
                var_dump($stream);
            });
        $stream->write($data);
        $stream->finalize_write($oid);
        $stream->free();
    };

    git_odb_foreach($srcodb,$lambda,[$srcodb,$odb]);
    echo 'Custom ODB has ' . $backend->count() . " entries.\n";
}

function test_session_backend_with_stream() {
    // Create custom backend. NOTE: This backend implements its own
    // writestream().
    $backend = new PHPSerializedODB_WithStream('odbcustom.test_session_backend_with_stream');
    $repo = git_repository_new();
    $odb = git_odb_new();

    git_odb_add_backend($odb,$backend,1);
    git_repository_set_odb($repo,$odb);

    // Create a blob in the repository.
    $data =<<<EOF
PHP is a general purpose programming language, especially well-suited for Web
development. It can be extended through writing extension modules that are either
compiled directly into the engine or loaded dynamically at runtime.
EOF;
    $oid = git_blob_create_frombuffer($repo,$data);
    echo "Created blob with oid=$oid\n";

    // Stream the objects from the test repository into our new repository.
    $srcrepo = git_repository_open_bare(testbed_get_repo_path());
    $srcodb = git_repository_odb($srcrepo);

    $lambda = function($oid,$store) {
        list($src,$dst) = $store;

        $obj = git_odb_read($src,$oid);
        $data = git_odb_object_data($obj);
        $type = git_odb_object_type($obj);

        $stream = git_odb_open_wstream($dst,strlen($data),$type);
        testbed_do_once('Git2Test.ODBCustom.transfer_object2',function() use($stream) {
                var_dump($stream);
            });
        $stream->write($data);
        $stream->finalize_write($oid);
        $stream->free();
    };

    git_odb_foreach($srcodb,$lambda,[$srcodb,$odb]);
    echo 'Custom ODB has ' . $backend->count() . " entries.\n";
}

function test_default_writepack() {
    global $HASH;

    // Create a new git repository and add a blob to its ODB.

    $dt = new DateTime;
    $tm = $dt->format('Y-m-d H:i:s');
    $data = "This is a blob created on $tm";

    $backend = new PHPSerializedODB('odbcustom.test_default_writepack');
    $repo = git_repository_new();
    $odb = git_odb_new();

    git_odb_add_backend($odb,$backend,1);
    git_repository_set_odb($repo,$odb);

    $stream = git_odb_open_wstream($odb,strlen($data),GIT_OBJ_BLOB);

    // The backend is not a direct PHPSerializedODB (since the git_odb resource
    // doesn't track it). But it will wind up calling into the methods of
    // $backend (which is a PHPSerializedODB).
    var_dump($stream->backend);

    $stream->write($data);
    $stream->finalize_write($HASH = sha1($data));
    $stream->free();

    echo "Wrote blob with ID=$HASH.\n";
}

function test_read_object() {
    global $HASH;

    $backend = new PHPSerializedODB('odbcustom.test_default_writepack');
    $repo = git_repository_new();
    $odb = git_odb_new();

    git_odb_add_backend($odb,$backend,1);
    git_repository_set_odb($repo,$odb);

    $blob = git_blob_lookup($repo,$HASH);
    var_dump(git_blob_rawcontent($blob));
}

testbed_test('Custom ODB/Copy (Default Stream)','\Git2Test\ODBCustom\test_session_backend');
testbed_test('Custom ODB/Copy (Custom Stream)','\Git2Test\ODBCustom\test_session_backend_with_stream');
testbed_test('Custom ODB/Default Writepack','\Git2Test\ODBCustom\test_default_writepack');
testbed_test('Custom ODB/Read Object','\Git2Test\ODBCustom\test_read_object');
