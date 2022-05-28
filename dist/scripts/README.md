This directory contains scripts samples.

## `unvanquished`

This Linux script behaves differently given it is named:

- `unvanquished`
- `unvanquished-server`
- `unvanquished-tty`

This script can be copied under those different names
to run the game as client, server or tty.

It reads a file named `/etc/unvanquished/paths.conf`
That configures distribution-specific paths. For example
on Debian it may contain:

```
system_libpath='/usr/lib/games/unvanquished'
system_pakpath='/usr/share/games/unvanquished/pkg'
system_homepath='/var/games/unvanquished-server'
```

- The `system_libpath` variable stores the path to the folder
containing the `daemon` binary.
- The `system_pakpath` variable stores the path to the folder
containing the `.dpk` archives.
- The `system_homepath` variable stores the path to the
system home directory.

If this file is absent, it assumes paths are relative the
folder storing this script.

If the `unvanquished-migrate-profile` script exists in `lib_path`
it will be executed before running the engine. Packagers are free
to not distribute this other script and the `unvanquished` script
will ignore it if absent.

## `unvanquished-migrate-profile`

This Linux script can move and migrate an old Unvanquished home
directory from an old Unvanquished version to the current place
and format. It must be stored in the `system_libpath` folder for
the `unvanquished` script to find it and run it.
