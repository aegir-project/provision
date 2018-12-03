#!/bin/sh -e

#
# Print changes for all packaged modules.
# This expects local checkouts to be available.
#
# To be used in the release notes on https://github.com/aegir-project/documentation/blob/3.x/docs/release-notes/

modules="hostmaster provision hosting eldir hosting_civicrm hosting_git hosting_remote_import hosting_site_backup_manager hosting_tasks_extra hosting_logs hosting_https"

if [ -z "$1" ]; then
  echo "Usage: $0 <previous release tag, e.g. 7.x-3.160>"
  echo "Best not to use the tags for minor releases, repositories that don't have this tag then fail to generate notes fails "
  exit 1
fi
prev_release=$1

CURRENT_BRANCH=7.x-3.x

TEMPDIR=`mktemp --directory`


echo "Cloning into temp dir $TEMPDIR..."
echo

cd $TEMPDIR

for shortname in $modules; do

  # Grab a fresh copy, to avoid projects being on feature branches and having local commits.
  git clone --quiet --branch $CURRENT_BRANCH git@git.drupal.org:project/$shortname.git >> $TEMPDIR/clone.log

  cd $shortname >> /dev/null;
  git pull --quiet
  echo "**Changes to $shortname since $prev_release**"
  changes=`drush rn --baseurl=https://www.drupal.org/ --md $prev_release HEAD | grep -v "Changes since $prev_release"`
  if [ -z "$changes" ]; then
    echo
    echo "* None"
  else
    echo "$changes"
  fi

  echo
  echo

  cd - >> /dev/null;
done

cd - >> /dev/null;

echo "Fresh clones left in temp dir $TEMPDIR for your conveniance."
