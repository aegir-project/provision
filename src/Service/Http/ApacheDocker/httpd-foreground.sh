#!/bin/bash

RED='\033[0;33m'
GRAY='\033[0;37m'
NC='\033[0m' # No Color

# Copied from official httpd container: https://github.com/docker-library/httpd/blob/fa5223d83a5225aa3fd5b23229b785c7764142bf/2.2/httpd-foreground

set -e
#
## Apache gets grumpy about PID files pre-existing
#rm -f /usr/local/apache2/logs/apache2.pid
#source /etc/apache2/envvars
#exec apache2 -DFOREGROUND

# Add symlink from our server's config to the apache include target.
echo "${RED}𝙋 ║${NC} Checking folder  ${GRAY}$AEGIR_ROOT/config${NC}"
ls -la $AEGIR_ROOT/config/

echo "${RED}𝙋 ║${NC} Checking folder  ${GRAY}$AEGIR_ROOT/config/$AEGIR_SERVER_NAME:${NC}"
ls -la $AEGIR_ROOT/config/$AEGIR_SERVER_NAME

# If there are no platforms assigned to the server, docker.conf and the docker config folders are never created.
#if [ ! -f '$AEGIR_ROOT/config/$AEGIR_SERVER_NAME/docker.conf' ]; then
#  touch $AEGIR_ROOT/config/$AEGIR_SERVER_NAME/docker.conf
#fi
#

echo "${RED}𝙋 ║${NC} Running ${GRAY}ln -sf $AEGIR_ROOT/config/$AEGIR_SERVER_NAME/apache.conf $AEGIR_ROOT/config/provision.conf${NC}"
ln -sf $AEGIR_ROOT/config/$AEGIR_SERVER_NAME/apache.conf $AEGIR_ROOT/config/provision.conf

echo "${RED}𝙋 ║${NC} Running ${GRAY}sudo /usr/sbin/apache2ctl start${NC}"
sudo /usr/sbin/apache2ctl start

echo "${RED}𝙋 ║${NC} Running ${GRAY}sudo /usr/sbin/apache2ctl start${NC}"
sudo /usr/sbin/apache2ctl start

echo "${RED}𝙋 ║${NC} Running ${GRAY}tail -f /var/log/aegir/system.log${NC}"
tail -f /var/log/aegir/system.log