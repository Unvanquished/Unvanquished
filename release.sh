#!/bin/sh

die()
{
	echo $@
	exit
}

START=ancestor
DEST=relpkg

test -d relbuild || die "relbuild dir not found"
cd relbuild
make --quiet || die "unable to compile"
cd -
rm -r $DEST
mkdir $DEST
for file in relbuild/*-stripped.nexe
do
	cp $file $DEST/$(basename $file -stripped.nexe).nexe
done

for file in $(git log --stat=150 --oneline --since "Sun Jan 29 18:30:00 2023 +0000" -- pkg | grep '|' | cut -d '|' -f1 | sort | uniq | cut -f1,2 --complement -d'/')
do
	mkdir -p "$DEST/$(dirname $file)"
	cp pkg/unvanquished_src.dpkdir/"$file" "$DEST/$file"
done

git log --oneline $START..HEAD | sed -n '/^[0-9a-f]* [A-Z]*:/ s!^[^ ]*\(.*\)!•<li>\1<br/></li>! p' >> $DEST/changelog
git format-patch --quiet -o $DEST/patches $START..HEAD
tar -c -f $DEST/patches.tar $DEST/patches
rm -r $DEST/patches
find $DEST -name '.*' -exec rm -r {} \+
rm release.dpk
cd $DEST
7z -tzip -mx=9 a ../release.dpk .
