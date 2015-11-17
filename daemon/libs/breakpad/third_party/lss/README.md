# Linux Syscall Support (LSS)

Every so often, projects need to directly embed Linux system calls instead of
calling the implementations in the system runtime library.

This project provides a header file that can be included into your application
whenever you need to make direct system calls.

## How to include linux\_syscall\_support.h in your project

You can either copy the file into your project, or preferably, you can set up
Git submodules to automatically pull from our source repository.

## Projects that use LSS

* [Chromium](https://www.chromium.org/)
* [Breakpad](https://chromium.googlesource.com/breakpad/breakpad)
* [Native Client](https://developer.chrome.com/native-client), in nacl\_bootstrap.c

## How to get an LSS change committed

### Review

You get your change reviewed, you can upload it to
[Rietveld](https://codereview.chromium.org)
using `git cl upload` from
[Chromium's depot-tools](http://dev.chromium.org/developers/how-tos/depottools).

### Testing

Unfortunately, LSS has no automated test suite.

You can test LSS by patching it into Chromium, building Chromium, and running
Chromium's tests.

You can compile-test LSS by running:

    gcc -Wall -Wextra -Wstrict-prototypes -c linux_syscall_support.h

### Rolling into Chromium

If you commit a change to LSS, please also commit a Chromium change to update
`lss_revision` in
[Chromium's DEPS](https://chromium.googlesource.com/chromium/src/+/master/DEPS)
file.

This ensures that the LSS change gets tested, so that people who commit later
LSS changes don't run into problems with updating `lss_revision`.
