#! /bin/bash

drush -y hostmaster-install --aegir_host=$1 --aegir_db_pass=$2
