#
# Install Aegir debian packages located in the projects unstable repository.
#
# This script is tuned for Debian 8 - Jessie.
#

echo "deb http://debian.aegirproject.org unstable main" | sudo tee -a /etc/apt/sources.list.d/aegir-unstable.list
curl http://debian.aegirproject.org/key.asc | sudo apt-key add -
sudo apt-get update
echo "debconf debconf/frontend select Noninteractive" | debconf-set-selections
#echo "debconf debconf/priority select critical" | debconf-set-selections


echo mysql-server-5.5 mysql-server/root_password password PASSWORD | debconf-set-selections
echo mysql-server-5.5 mysql-server/root_password_again password PASSWORD | debconf-set-selections

debconf-set-selections <<EOF
aegir3-hostmaster aegir/db_password string PASSWORD
aegir3-hostmaster aegir/db_password seen  true
aegir3-hostmaster aegir/db_user string root
aegir3-hostmaster aegir/db_host string localhost
aegir3-hostmaster aegir/email string  aegir@example.com
aegir3-hostmaster aegir/site  string  aegir.example.com
postfix postfix/main_mailer_type select Local only

EOF

sudo DPKG_DEBUG=developer apt-get install --yes aegir3 mysql-server


