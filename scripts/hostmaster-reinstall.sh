#! /bin/bash

/var/aegir/.drush/provision/hostmaster-purge.sh $1
/var/aegir/.drush/provision/hostmaster-dev-install.sh $1 $2
