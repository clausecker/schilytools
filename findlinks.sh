#!/bin/sh

# generate .gitignore files from MKLINKS and update .links

LC_COLLATE=C; export LC_COLLATE

links=`mktemp .links.XXXXXX`
psmakels=`mktemp psmakels.XXXXXX`

for mklinks in `find . -name MKLINKS`
do
	dir="`dirname "$mklinks"`"
	[ -f $dir/MKLINKS ] || continue
	grep -q ^\$symlink $dir/MKLINKS || continue

	echo '# generated by mkgitignore.sh, do not edit' >$dir/.gitignore
	echo >>$dir/.gitignore
	grep ^\$symlink $dir/MKLINKS | while read _ src dest
	do
		if [ "$dest" = . ]
		then
			destfile="`basename "$src"`"
			echo "$destfile" >>$dir/.gitignore
			echo "$dir/$destfile" >>$links
		else
			echo "$dest" >>$dir/.gitignore
			echo "$dir/$dest" >>$links
		fi
	done
done

# special handling for psmake
(cd psmake && sh rmlinks)
touch psmake/.gitignore
find ./psmake | sort >$psmakels
(cd psmake && sh lnfiles)
find ./psmake | sort | comm -13 $psmakels - >>$links
find ./psmake | sort | comm -13 $psmakels - | sed -e 's,^./psmake/,,' >psmake/.gitignore

env LC_COLLATE=C sort $links >.links

rm -f $links $psmakels
