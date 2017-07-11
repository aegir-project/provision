# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT

sudo apt-get update
sudo apt-get install --yes curl

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

sudo apt-get install --yes libapache2-mod-php5 mysql-server apache2 php5-mysql php5-gd git-core git

#sudo DPKG_DEBUG=developer apt-get install --yes aegir3

echo
echo "Now install packages with sudo dpkg -i /vagrant/*.deb, copy them from the parent dir and run with 'vagrant ssh'"
echo "See the docs on http://docs.aegirproject.org/en/3.x/community/release-process/ on how to build the debian packages"

SCRIPT

Vagrant::Config.run do |config|
  config.vm.box = "debian/jessie64"

  config.vm.host_name = "aegir3-deb-unstable-no-puppet.test"

  config.vm.provision "shell", inline: $script

  # We can speed up subsequent rebuilds by caching the apt cache directories
  # on the host machine.
  config.vm.share_folder("apt_cache", "/var/cache/apt/archives", "tmp/apt/cache", :create => true)

end
