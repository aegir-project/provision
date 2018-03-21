#!/usr/bin/env bash

PREFIX='𝙋𝙍𝙊𝙑𝙄𝙎𝙄𝙊𝙉 entrypoint.sh ║'
echo "$PREFIX Started httpd-foreground.sh ..."

echo "$PREFIX Running ln -sf /var/$USER_NAME/config/$SERVER_NAME/apacheDocker.conf /var/$USER_NAME/config/provision.conf"
ln -sf /var/$USER_NAME/config/$SERVER_NAME/apacheDocker.conf /var/$USER_NAME/config/provision.conf

echo "$PREFIX Running sudo /usr/sbin/apache2ctl start"
sudo /usr/sbin/apache2ctl start

echo "$PREFIX Running tail -f /var/log/$USER_NAME.log"
tail -f /var/log/$USER_NAME.log