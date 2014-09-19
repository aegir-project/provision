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
major="7.x"

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
release $major-$version.
EOF

cat <<EOF

The following operations will be done:
 0. prompt you for a debian/changelog entry
 1. change the makefile to download tarball
 2. change the upgrade.sh.txt version
 3. display the resulting diff
 4. commit those changes to git
 5. lay down the tag
 6. revert the commit
 7. clone fresh copies of hosting/hostmaster and eldir to lay down the tag
 8. (optionally) push those changes

The operation can be aborted before step 8. Don't forget that as
long as changes are not pushed upstream, this can all be reverted (see
git-reset(1) and git-revert(1) ).

EOF

if ! prompt_yes_no "continue?" ; then
    exit 1
fi

git pull --rebase

debversion=$(echo $version | sed -e 's/-/~/')
dch -v $debversion -D unstable
git add debian/changelog

echo changing hostmaster version in aegir-release.make
sed -i'.tmp' -e '/^projects\[hostmaster\]\[version\]/s/=.*$/= "'"$major-$version"'"/' aegir-release.make && git add aegir-release.make && rm aegir-release.make.tmp

echo enabling release makefilexs
ln -sf aegir-release.make aegir.make && git add aegir.make

echo changing provision.info version
sed -i'.tmp' -e"s/version *=.*$/version=$major-$version/" provision.info
git add provision.info && rm provision.info.tmp

echo changing upgrade.sh.txt version
sed -i'.tmp' -e"s/AEGIR_VERSION=.*$/AEGIR_VERSION=\"$major-$version\"/" upgrade.sh.txt && git add upgrade.sh.txt && rm upgrade.sh.txt.tmp

echo resulting changes to be committed:
git diff --cached | cat

if prompt_yes_no "commit changes and tag release?"; then
    echo okay, committing...
else
    echo 'aborting, leaving changes in git staging area'
    echo 'use "git reset --hard" to revert the index'
    exit 1
fi

CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
NEW_TAG="$major-$version"
commitmsg=`git commit -m"change version information for release $version"`
echo $commitmsg
commitid=`echo $commitmsg | sed 's/^\[[^ ]* \([a-z0-9]*\)\].*$/\1/'`
sed -n '1,/ --/p' debian/changelog | git tag -a -F - $major-$version

echo reverting tree to HEAD versions
git revert --no-commit $commitid
# Unstage the debian/changelog change, as we don't want to revert that.
git reset --quiet HEAD 'debian/changelog'
git checkout -- 'debian/changelog'
git commit


echo "Work on the other project repositories."
mkdir -p build-area;

# Hostmaster
rm -rf build-area/hostmaster
git clone --branch $CURRENT_BRANCH `git config remote.origin.url | sed 's/provision/hostmaster/'` build-area/hostmaster

echo "Setting the tag $NEW_TAG in a clean hostmaster clone."
git --work-tree=build-area/hostmaster --git-dir=build-area/hostmaster/.git tag -a $NEW_TAG -m 'Add a new release tag.'

# Hosting
rm -rf build-area/hosting
git clone --branch $CURRENT_BRANCH `git config remote.origin.url | sed 's/provision/hosting/'` build-area/hosting

echo "Setting the tag $NEW_TAG in a clean hosting clone."
git --work-tree=build-area/hosting --git-dir=build-area/hosting/.git tag -a $NEW_TAG -m 'Add a new release tag.'

# Eldir
rm -rf build-area/eldir
git clone --branch $CURRENT_BRANCH `git config remote.origin.url | sed 's/provision/eldir/'` build-area/eldir

echo "Setting the tag $NEW_TAG in a clean eldir clone."
git --work-tree=build-area/eldir --git-dir=build-area/eldir/.git tag -a $NEW_TAG -m 'Add a new release tag.'


# Can we push?
if prompt_yes_no "push tags and commits upstream? "; then
    # this makes sure we push the commit *and* the tag
    git push --tags origin HEAD
    git --work-tree=build-area/hostmaster --git-dir=build-area/hostmaster/.git push --tags origin HEAD
    git --work-tree=build-area/hosting --git-dir=build-area/hosting/.git push --tags origin HEAD
    git --work-tree=build-area/eldir --git-dir=build-area/eldir/.git push --tags origin HEAD
fi
