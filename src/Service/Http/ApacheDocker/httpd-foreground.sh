#!/bin/bash
echo "𝙋𝙍𝙊 ║ Started httpd-foreground.sh ..."

# Copied from official httpd container: https://github.com/docker-library/httpd/blob/fa5223d83a5225aa3fd5b23229b785c7764142bf/2.2/httpd-foreground

set -e
#
## Apache gets grumpy about PID files pre-existing
#rm -f /usr/local/apache2/logs/apache2.pid
#source /etc/apache2/envvars
#exec apache2 -DFOREGROUND

echo "𝙋𝙍𝙊 ║ Checking folder  /etc/apache2/conf-available"
ls -la /etc/apache2/conf-available

echo "𝙋𝙍𝙊 ║ Checking folder  /etc/apache2/conf-enabled"
ls -la /etc/apache2/conf-enabled

# Add symlink from our server's config to the apache include target.
echo "𝙋𝙍𝙊 ║ Checking folder  $AEGIR_ROOT/config"
ls -la $AEGIR_ROOT/config/

echo "𝙋𝙍𝙊 ║ Checking folder $AEGIR_ROOT/config/$AEGIR_SERVER_NAME:"
ls -la $AEGIR_ROOT/config/$AEGIR_SERVER_NAME

# If there are no platforms assigned to the server, docker.conf and the docker config folders are never created.
#if [ ! -f '$AEGIR_ROOT/config/$AEGIR_SERVER_NAME/apacheDocker.conf' ]; then
#  touch $AEGIR_ROOT/config/$AEGIR_SERVER_NAME/apacheDocker.conf
#fi
#

echo "𝙋𝙍𝙊 ║ Running ln -sf $AEGIR_ROOT/config/$AEGIR_SERVER_NAME/apacheDocker.conf $AEGIR_ROOT/config/provision.conf"
ln -sf $AEGIR_ROOT/config/$AEGIR_SERVER_NAME/apacheDocker.conf $AEGIR_ROOT/config/provision.conf

echo "𝙋𝙍𝙊 ║ Running sudo /usr/sbin/apache2ctl start"
sudo /usr/sbin/apache2ctl start

echo "𝙋𝙍𝙊 ║ Running tail -f /var/log/aegir/system.log"
tail -f /var/log/aegir/system.log