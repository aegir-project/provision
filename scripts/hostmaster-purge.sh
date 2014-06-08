#! /bin/bash

drush -y @hostmaster hostmaster-uninstall
rm -rf /var/aegir/backups
rm -rf /var/aegir/clients
rm -rf /var/aegir/platforms
rm -rf /var/aegir/config
rm -rf /var/aegir/hostmaster-7.x-3.x/sites/$1
rm -rf /var/aegir/hostmaster-7.x-3.x
rm -rf /var/aegir/.drush/*.alias.drushrc.php
