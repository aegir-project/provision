#
# Install Aegir debian packages located in the 'build/' directory.
# These are provided by the GitLab CI build stage.
#
# This script is tuned for Debian 9 - Stretch.
#

echo "[CI] Updating APT"
sudo apt-get update

echo "[CI] Setting debconf settings"
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
echo "[CI] Pre-installing dependencies"
sudo apt-get install --yes php7.0-mysql php7.0-cli


echo "[CI] Installing .deb files .. will fail on missing packages"
sudo DPKG_DEBUG=developer dpkg --install build/aegir3-cluster-slave_*.deb

echo "[CI] Installing remaining packages and configuring our debs"
sudo apt-get install --fix-broken --yes



