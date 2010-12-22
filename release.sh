#!/bin/sh -e

# simple prompt, copied from install.sh
prompt_yes_no() {
  while true ; do
    printf "$* [Y/n] "
    read answer
    if [ -z "$answer" ] ; then
      return 0
    fi
    case $answer in
      [Yy]|[Yy][Ee][Ss])
        return 0
        ;;
      [Nn]|[Nn][Oo])
        return 1
        ;;
      *)
        echo "Please answer yes or no"
        ;;
    esac
 done 
}

version=$1
old_version=$2

if [ $# -lt 1 -o "$version" = "-h" ]; then
    cat <<EOF 
not enough arguments

Usage: $0 <new_version> [ <old_version> ]
EOF
    exit 1
fi

cat <<EOF
Aegir release script
====================

This script should only be used by the core dev team when doing an
official release. If you are not one of those people, you probably
shouldn't be running this.

This script is going to modify the configs and documentation to
release $version from release $old_version.
EOF

if [ -z "$old_version" ]; then
    echo "warning: no old version specified, UPGRADE.txt will be partly updated"
fi

cat <<EOF

The following operations will be done:
 1. change the makefile to download tarball
 2. change the INSTALL.txt to point to tagged install.sh
 3. change the UPGRADE.txt to point to release tags
 4. change the install.sh.txt version
 5. change the upgrade.sh.txt version
 6. display the resulting diff
 7. commit those changes to git
 8. lay down the tag (prompting you for a changelog)
 9. revert the commit
 10. (optionally) push those changes

The operation can be aborted before step 6 and 9. Don't forget that as
long as changes are not pushed upstream, this can all be reverted (see
git-reset(1) and git-revert(1) ).

EOF

if ! prompt_yes_no "continue?" ; then
    exit 1
fi

git pull
echo changing makefile to download tarball
sed -i'.tmp' -e'/^projects\[hostmaster\]\[download\]\[type\]/s/=.*$/ = "get"/' \
  -e'/^projects\[hostmaster\]\[download\]\[url\]/s#=.*$#= "http://files.aegirproject.org/hostmaster-'$version'.tgz"#' \
  -e'/^projects\[hostmaster\]\[download\]\[branch\].*/s/\[branch\] *=.*$/[directory_name] = "hostmaster"/' aegir.make && git add aegir.make && rm aegir.make.tmp

echo changing INSTALL.txt to point to tagged install.sh
sed -i'.tmp' -e"/http:\/\/git.aegirproject.org\/?p=provision.git;a=blob_plain;f=install.sh.txt;hb=HEAD/s/HEAD/provision-$version/" docs/INSTALL.txt && git add docs/INSTALL.txt && rm docs/INSTALL.txt.tmp

echo changing hostmaster-install version
sed -i'.tmp' -e"s/version *=.*$/version=$version/" provision.info
git add provision.info && rm provision.info.tmp

echo changing UPGRADE.txt to point to tagged upgrade.sh
sed -i'.tmp' -e"/http:\/\/git.aegirproject.org\/?p=provision.git;a=blob_plain;f=upgrade.sh.txt;hb=HEAD/s/HEAD/provision-$version/" docs/UPGRADE.txt && git add docs/UPGRADE.txt && rm docs/UPGRADE.txt.tmp

echo changing UPGRADE.txt to point to release tags
sed -i'.tmp' -e"s/export AEGIR_VERSION=HEAD/export AEGIR_VERSION=$version/" docs/UPGRADE.txt

if ! [ -z "$old_version" ]; then
    sed -i.tmp -e"/export OLD_DRUPAL_DIR=/s#hostmaster-.*#hostmaster-$old_version#" docs/UPGRADE.txt
fi
git add docs/UPGRADE.txt && rm docs/UPGRADE.txt.tmp

echo changing install.sh.txt version
sed -i'.tmp' -e"s/AEGIR_VERSION=.*$/AEGIR_VERSION=\"$version\"/" install.sh.txt && git add install.sh.txt && rm install.sh.txt.tmp

echo changing upgrade.sh.txt version
sed -i'.tmp' -e"s/AEGIR_VERSION=.*$/AEGIR_VERSION=\"$version\"/" upgrade.sh.txt && git add upgrade.sh.txt && rm upgrade.sh.txt.tmp

echo resulting changes to be committed:
git diff --cached | cat

if prompt_yes_no "commit changes and tag release? (y/N) "; then
    echo okay, committing...
else
    echo 'aborting, leaving changes in git staging area'
    echo 'use "git reset; git checkout ." to revert'
    exit 1
fi

commitmsg=`git commit -m"change version information for release $version, from $old_version"`
echo $commitmsg
commitid=`echo $commitmsg | sed 's/^\[[a-z]* \([a-z0-9]*\)\].*$/\1/'`
git tag -a provision-$version

echo reverting tree to HEAD versions
git revert $commitid

if prompt_yes_no "push tags and commits upstream? (y/N) "; then
    # this makes sure we push the commit *and* the tag
    git push --tags origin HEAD
fi
