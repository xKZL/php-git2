php-git2
========

This project provides PHP bindings for `libgit2`. The extension dynamically
links to libgit2 installed in the usual way on your system (but you can static
link it if you want). We also expect the git2 headers to be installed in the
usual way.

The extension currently targets PHP 5 (see Versioning) and has been tested with
PHP 5.4, 5.5 and 5.6.

Primary author:

    Roger Gee <rpg11a@acu.edu>

Versioning
----------

Since we do not bundle the `libgit2` dependency, it up to the user to provide the
correct version of `libgit2` required by their version of php-git2 (either as a
shared or static library). If you are dynamic linking and on Linux, php-git2
will embed the soname of whatever git2 version you have installed embedded into
it, so it will always load the correct version. To avoid ambiguity, we have
provided a listing of the version of git2 covered by php-git2:

| php-git2 | libgit2 | PHP           |
| -------- | ------- | ------------- |
| current  | v0.25.1 |  >=5.4 but <7 |

Currently `php-git2` targets only PHP 5.

API Coverage
------------

We document API coverage in the [DOCS](DOCS) file, including the major
differences between the PHP userspace API and the original C library API. Please
consult this documentation before using these bindings, as there are some
differences that need to be understood.

The [bindings](bindings) file contains a list of all bindings and whether they
are covered or not.

Please make sure to update these files when adding/updating functionality.

Design
------

Our core principle is to stick as closely as possible to the original, C
`libgit2` API while at the same time making the extension behave comfortably in
userspace with what PHP devs expect. At times we add some calls to facilitate
certain tasks in userspace or to optimize an otherwise costly operation
(e.g. like converting values back-and-forth between userspace and the
library). We also provide several abstractions as PHP classes so that they can
be subclassed from userspace (e.g. the `git_odb_backend` is a class called
`GitODBBackend`).

#### Stability

We want the extension to be stable so that it can be used securely in production
code. To that end we've added an extra reference counting layer to track
resource dependencies. This ensures PHP scripters don't blow their feet off when
doing something like this:

```php
function get_ref() {
    $repo = git_repository_open('/path/to/repo.git',true);
    $ref = git_reference_lookup($repo,'annot-tag-1');
    return $ref;
}

$ref = get_ref();
$peeled = git_reference_peel($ref,GIT_OBJ_COMMIT);
```

The extension will keep the `git_repository` object alive behind the scenes
since a dependency is established between the `git_repository` and the
`git_reference`. This prevents the extension (and PHP) from crashing when users
do crazy wack.

#### Programming methodology

Most of the extension is designed as inline code in header files. We use C++
metaprogramming constructs to generate extension functions. This approach is
great for knocking out redundant tasks, separating the prototype for a binding
from its implementation and keeping track of API changes. However it comes with
the small drawback of decreased flexibility when modifying a binding.

If a binding doesn't "fit the mold" and cannot be implemented using one of the
generic binding template function generators, then we recommend the binding be
written directly in the header file using the conventional `PHP_FUNCTION` macro.

We write PHP class implementations directly in their own compilation units since
not too much of that code can be generalized.

Building
--------

This extension is written in C++, and it takes advantage of several C++11
features. You need to have a relatively recent C++ compiler that supports C++11
metaprogramming features. This extension has been built successfully using GCC
and clang on macOS and Linux. Experimental versions for PHP5 have been built on
Windows using MinGW (see section on Windows).

Build the extension in the normal way using autoconf and make. Ensure you build
the correct version of php-git2 corresponding to whatever git2 you have
installed on the system. If you want to static link git2 into the extension,
install the archive library for git2 in place of the dynamic link one (or alter
the compile macros in the config file to point at your static archive).

When you configure the build, there are a couple of different options you should
consider to make the binary useful for your particular purpose. Add options to
the CXXFLAGS variable since the C++ compiler is used for all compilation
units. For release builds, enable optimizations like so:

    ./configure CXXFLAGS='-O3'

Since the code uses a lot of inline template functions, optimizations will
really help improve the performance of the binary (and makes it a lot
smaller). However this is not great for testing since it collapses function
calls down. For a development environment, we recommend something like

    ./configure CXXFLAGS='-g -O0 -Wfatal-errors -Wall -Wno-deprecated-register'

This configuration will produce a huge binary, but it will be easier to
trace. If you are developing, we also recommend including the
[Makefile.extra](Makefile.extra) at the bottom of the generated `Makefile`. This
will give the build system some dependency information that makes life easier.

Windows
-------

This project does not officially support Windows at this time, due to compiler
version requirements for PHP5 and inadaquete C++11 support in those
compilers. Experimental builds have been built with MinGW which work
successfully. However these builds required a lot of hacking and were designed
to only be used in _testing_, not production, environments.

If you are building for Windows, or using one of our experimental builds, make
sure that your `libgit2` does not static link to the CRT. It needs to dynamic
link to the same runtime being used by `php5` and `php-git2`.

In our experimental builds, we were able to successfully build `php-git2.dll`
using MinGW (though we had to hack the standard and PHP headers a little). The
resulting library linked against `msvcr100.dll`, and we built PHP 5.4 and `git2`
with the VC100 tools (which is non-standard).
