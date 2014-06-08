#! /bin/bash

/var/aegir/.drush/provision/scripts/hostmaster-purge.sh $1
/var/aegir/.drush/provision/scripts/hostmaster-platform.sh
/var/aegir/.drush/provision/scripts/hostmaster-dev-install.sh $1 $2
