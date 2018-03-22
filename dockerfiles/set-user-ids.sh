#!/usr/bin/env bash

# Usage:
# set-user-ids NAME UID GID
#
set -e

PREFIX='𝙋𝙍𝙊𝙑𝙄𝙎𝙄𝙊𝙉 set-user-ids.sh ║'

USER_NAME=$1
USER_UID=$2
WEB_UID=$3

echo "$PREFIX Changing user '$USER_NAME' UID/GID to '$USER_UID'...
"
usermod -u $USER_UID $USER_NAME
groupmod -g $USER_UID $USER_NAME

echo "$PREFIX Changing user 'www-data' UID/GID to '$WEB_UID'...
"
usermod -u $WEB_UID www-data
groupmod -g $WEB_UID www-data
