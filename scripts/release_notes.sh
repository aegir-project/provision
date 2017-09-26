#!/bin/sh -e

#
# Print changes for all packaged modules.
# This expects local checkouts to be available.
#
# To be used in the release notes on https://github.com/aegir-project/documentation/blob/3.x/docs/release-notes/

modules="hostmaster provision hosting eldir hosting_civicrm hosting_git hosting_remote_import hosting_site_backup_manager hosting_tasks_extra"

if [ -z "$1" ]; then
  echo "Usage: $0 <previous release tag>"
  exit 1
fi
prev_release=$1

cd ..

for shortname in $modules; do
  cd $shortname >> /dev/null;
  git pull --quiet
  echo "**Changes to $shortname since $prev_release**"
  drush rn --baseurl=https://www.drupal.org/ --md $prev_release HEAD | grep -v "Changes since $prev_release"
  cd - >> /dev/null;
done

cd -
