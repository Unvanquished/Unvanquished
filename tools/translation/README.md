# Unvanquished translation string generator

Once committed and pushed, the translation files are automatically pulled by
the Weblate translation tool hosted at:

- https://hosted.weblate.org/projects/unvanquished/unvanquished/

The `generate.sh` script is the entry point. It is recommended to run it after
merging translation changes from Weblate.

It runs the `generate_pot.sh` script that generate the `.pot` files containing
the strings from the source code that need to be translated from English to
other languages.

It then runs the `update_po.sh` script to merge them in `.po` files containing
the translated strings. The `update_po.sh` only updates existing `.po` file,
adding a new `.po` file for a new language must be done separately, the Weblate
tool does it automatically when adding a language in Weblate.

The `generate.sh` script always run the `update_po.sh` script after running
the `generate_pot.sh` script as there have been doubt Weblate only merged
the updated strings from `.pot` files even if such updated strings existed
in `.po` files. In case it's not needed anymore it can't do harm anyway, and
it makes the `.po` files ready to use outside of Weblate.

