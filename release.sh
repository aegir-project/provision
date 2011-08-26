#!/bin/sh -e

# simple prompt
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

if [ $# -lt 1 -o "$version" = "-h" ]; then
    cat <<EOF 
not enough arguments

Usage: $0 <new_version>
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
release $version.
EOF

cat <<EOF

The following operations will be done:
 1. change the makefile to download tarball
 2. change the upgrade.sh.txt version
 3. display the resulting diff
 4. commit those changes to git
 5. lay down the tag (prompting you for a changelog)
 6. revert the commit
 7. (optionally) push those changes

The operation can be aborted before step 7. Don't forget that as
long as changes are not pushed upstream, this can all be reverted (see
git-reset(1) and git-revert(1) ).

EOF

if ! prompt_yes_no "continue?" ; then
    exit 1
fi

git pull
echo changing makefile to download tarball
#sed -i'.tmp' -e'/^projects\[hostmaster\]\[download\]\[type\]/s/=.*$/ = "get"/' \
#  -e'/^projects\[hostmaster\]\[download\]\[url\]/s#=.*$#= "http://ftp.drupal.org/files/projects/hostmaster-'$version'.tgz"#' \
#  -e'/^projects\[hostmaster\]\[download\]\[branch\].*/s/\[branch\] *=.*$/[directory_name] = "hostmaster"/' aegir.make && git add aegir.make && rm aegir.make.tmp
sed -i'.tmp' -e'/^projects\[hostmaster\]\[download\]\[type\]/s/=.*$/= "git"/' \
  -e'/^projects\[hostmaster\]\[download\]\[url\]/s#=.*$#= "http://git.drupal.org/project/hostmaster.git"#' \
  -e'/^projects\[hostmaster\]\[download\]\[branch\].*/s/\[branch\] *=.*$/[tag] = "'$version'"/' aegir.make && git add aegir.make && rm aegir.make.tmp

echo changing provision.info version
sed -i'.tmp' -e"s/version *=.*$/version=$version/" provision.info
git add provision.info && rm provision.info.tmp

echo changing upgrade.sh.txt version
sed -i'.tmp' -e"s/AEGIR_VERSION=.*$/AEGIR_VERSION=\"$version\"/" upgrade.sh.txt && git add upgrade.sh.txt && rm upgrade.sh.txt.tmp

echo resulting changes to be committed:
git diff --cached | cat

if prompt_yes_no "commit changes and tag release?"; then
    echo okay, committing...
else
    echo 'aborting, leaving changes in git staging area'
    echo 'use "git reset --hard" to revert the index'
    exit 1
fi

commitmsg=`git commit -m"change version information for release $version"`
echo $commitmsg
commitid=`echo $commitmsg | sed 's/^\[[^ ]* \([a-z0-9]*\)\].*$/\1/'`
git tag -a $version

echo reverting tree to HEAD versions
git revert $commitid

if prompt_yes_no "push tags and commits upstream? "; then
    # this makes sure we push the commit *and* the tag
    git push --tags origin HEAD
fi
