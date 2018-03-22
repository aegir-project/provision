#!/usr/bin/env bash

PREFIX='𝙋𝙍𝙊𝙑𝙄𝙎𝙄𝙊𝙉 entrypoint.sh ║'
echo "$PREFIX Started entrypoint.sh ..."

echo "$PREFIX Running ln -sf /var/$PROVISION_USER_NAME/config/$SERVER_NAME/apacheDocker.conf /var/$PROVISION_USER_NAME/config/apache.conf"
ln -sf /var/$PROVISION_USER_NAME/config/$SERVER_NAME/apacheDocker.conf /var/$PROVISION_USER_NAME/config/apache.conf

echo "$PREFIX Running sudo /usr/sbin/apache2ctl start"
sudo /usr/sbin/apache2ctl start

echo "$PREFIX Running tail -f /var/log/$PROVISION_USER_NAME.log"
tail -f /var/log/$PROVISION_USER_NAME.log