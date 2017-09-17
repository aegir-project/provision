#
# Install Aegir debian packages located in the 'build/' directory.
# These are provided by the GitLab CI build stage.
#
# This script is tuned for Debian 9 - Stretch.
#

sudo apt-get update
echo "debconf debconf/frontend select Noninteractive" | sudo debconf-set-selections



sudo debconf-set-selections <<EOF
aegir3-hostmaster aegir/db_password string PASSWORD
aegir3-hostmaster aegir/db_password seen  true
aegir3-hostmaster aegir/db_user string aegir_root
aegir3-hostmaster aegir/db_host string localhost
aegir3-hostmaster aegir/email string  aegir@example.com
aegir3-hostmaster aegir/site  string  aegir.example.com
postfix postfix/main_mailer_type select Local only

EOF

set -x
sudo apt-get install --yes mariadb-server-10.1 php7.0-mysql php7.0-cli


sudo DPKG_DEBUG=developer dpkg --install build/aegir3_*.deb build/aegir3-provision*.deb build/aegir3-hostmaster*.deb
sudo apt-get install --fix-broken --yes



