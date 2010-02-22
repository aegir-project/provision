#! /bin/sh

# $Id$

########################################################################
# Aegir quick install script
#
# This script takes care of deploying all the required PHP scripts for
# the backend to run properly. It should be ran as the Aegir user.
#
# It should keep to strict POSIX shell syntax to ensure maximum
# portability. The aim here is to ease the burden on porters but also
# allow people using various platforms to zip through the install quicker.
#
# This script should be useable by packaging scripts to avoid code duplication,
# so it also needs to be idem-potent (ie. that it doesn't break if ran multiple
# times).
#
# This script also *DOES NOT CHECK* if the requirements described in
# INSTALL.txt have been met.  It's up to the admin to follow the proper install
# instructions or use the packages provided by their platform.
########################################################################
# This script takes the following steps:
#
# 1. parse commandline
# 2. creates a basic directory structure in $AEGIR_HOME
# 3. downloads drush in $AEGIR_HOME
# 4. downloads drush_make in $AEGIR_HOME/.drush
# 5. downloads provision in $AEGIR_HOME/.drush
# 6. creates an apache config file in $AEGIR_HOME/config/apache.conf
########################################################################
# basic variables, change before release
AEGIR_HOME=$HOME
WEB_GROUP=www-data
DRUSH_VERSION=6.x-3.0-alpha1
DRUSH_MAKE_VERSION=6.x-2.0-beta6

# when adding a variable here, add it to the display below

########################################################################
# functions

# noticeable messages
msg() {
  echo "==> $*"
}

# simple prompt
prompt_yes_no() {
  while true ; do
    printf "$* [Y/n] "
    read answer
    if [ -z "$answer" ] ; then
      return 0
    fi
    case $answer in
      [Yy]|[Yy][Ee][Ss])
        return 0
        ;;
      [Nn]|[Nn][Oo])
        return 1
        ;;
      *)
        echo "Please answer yes or no"
        ;;
    esac
 done 

}

usage() {
  cat <<EOF
Usage: $0 [ -h ] [ -w group ] [ -d home ]
EOF
}

########################################################################
# Main script

# stop on error
set -e

# parse commandline
args=`getopt V:w:d:h $*`
set -- $args

for i
do
  case "$i" in
    -w) shift; WEB_GROUP=$1; shift;;
    -h) shift; usage; exit;;
    -d) shift; AEGIR_HOME=$1; shift;;
    --) shift; break;;
  esac
done

DRUSH="$AEGIR_HOME/drush/drush.php"

msg "Aegir automated install script"

if [ `whoami` = "root" ] ; then
  msg "This script should be ran as a non-root user"
  exit 1
fi

msg "Configuring provision backend with the following settings:"
cat <<EOF
AEGIR_HOME=$AEGIR_HOME
WEB_GROUP=$WEB_GROUP
DRUSH=$DRUSH
DRUSH_VERSION=$DRUSH_VERSION
EOF

msg "Creating basic directory structure"
mkdir -p $AEGIR_HOME/config/vhost.d
mkdir -p $AEGIR_HOME/backups
chmod 0711 $AEGIR_HOME/config
chmod 0700 $AEGIR_HOME/backups

# we need to check both because some platforms (like SunOS) return 0 even if the binary is not found
if which drush 2> /dev/null && which drush | grep -v 'no drush in' > /dev/null; then
  msg "Drush is in the path, good"
  DRUSH=drush
elif [ -x $DRUSH ] ; then
  msg "Drush found in $DRUSH, good"
  DRUSH="php $AEGIR_HOME/drush/drush.php"
else
  msg "Installing drush in $AEGIR_HOME"
  cd $AEGIR_HOME
  wget http://ftp.drupal.org/files/projects/drush-$DRUSH_VERSION.tar.gz
  gunzip -c drush-$DRUSH_VERSION.tar.gz | tar -xf -
  rm drush-$DRUSH_VERSION.tar.gz
  DRUSH="php $AEGIR_HOME/drush/drush.php"
fi

if $DRUSH help > /dev/null ; then
  msg "Drush seems to be functionning properly"
else
  msg "Drush is broken ($DRUSH help failed)"
  exit 1
fi

if $DRUSH help | grep "^ make" > /dev/null ; then
  msg "Drush make already seems to be installed"
else
  msg "Installing drush make in $AEGIR_HOME/.drush"
  mkdir -p $AEGIR_HOME/.drush
  $DRUSH dl drush_make-$DRUSH_MAKE_VERSION --destination=$AEGIR_HOME/.drush
fi

if $DRUSH help | grep "^ provision install" > /dev/null ; then
  msg "Provision already seems to be installed"
else
  msg "Installing provision backend in $AEGIR_HOME/.drush"
  mkdir -p $AEGIR_HOME/.drush
  if [ "$AEGIR_VERSION" = "HEAD" ]; then
    git clone -q git://git.aegirproject.org/provision $AEGIR_HOME/.drush/provision
  else
    cd $AEGIR_HOME/.drush
    wget http://files.aegirproject.org/provision-$AEGIR_VERSION.tgz
    gunzip -c provision-$AEGIR_VERSION.tgz | tar -xf -
    rm provision-$AEGIR_VERSION.tgz
  fi
fi

if [ -f $AEGIR_HOME/apache.conf ]; then
  cat > $AEGIR_HOME/apache.conf <<EOF
NameVirtualHost *:80  
NameVirtualHost *:443  

<IfModule !env_module>
  LoadModule env_module modules/mod_env.so
</IfModule>

<IfModule !rewrite_module>
  LoadModule rewrite_module modules/mod_rewrite.so
</IfModule>

Include /var/aegir/config/vhost.d/
EOF
fi

msg "Aegir provision backend installed successfully"
