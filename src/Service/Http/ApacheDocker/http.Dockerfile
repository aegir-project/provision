FROM ubuntu:14.04

# Use --build-arg option when running docker build to set these variables.
# If wish to "mount" a volume to your host, set AEGIR_UID and AEGIR_GID to your local user's UID.
# There are both ARG and ENV lines to make sure the value persists.
# See https://docs.docker.com/engine/reference/builder/#/arg

# The UID and GID for the aegir user.
ARG AEGIR_UID=1000
ENV AEGIR_UID ${AEGIR_UID:-1000}

ARG APACHE_UID=10000
ENV APACHE_UID ${APACHE_UID:-10000}

# The home directory for the aegir user.
ARG AEGIR_ROOT=/var/aegir
ENV AEGIR_ROOT ${AEGIR_ROOT:-/var/aegir}

# The string used to create /var/aegir/config/SERVER_NAME folders
ARG AEGIR_SERVER_NAME=server_master
ENV AEGIR_SERVER_NAME ${AEGIR_SERVER_NAME:-server_master}

RUN echo "Changing user www-data to UID $APACHE_UID and GID $APACHE_UID..."
RUN usermod -u $APACHE_UID www-data
RUN groupmod -g $APACHE_UID www-data

RUN apt-get -qq -o Dpkg::Use-Pty=0 update && DEBIAN_FRONTEND=noninteractive apt-get -qq -o Dpkg::Use-Pty=0 install \
  apache2 \
  php5 \
  php5-cli \
  php5-gd \
  php5-mysql \
  php-pear \
  php5-curl \
  postfix \
  sudo \
  rsync \
  git-core \
  unzip \
  wget \
  mysql-client \
  tree

RUN echo "Creating user aegir with UID $AEGIR_UID and GID $AEGIR_GID and HOME $AEGIR_ROOT ..."

RUN addgroup --gid $AEGIR_UID aegir
RUN adduser --uid $AEGIR_UID --gid $AEGIR_UID --system --home $AEGIR_ROOT aegir
RUN adduser aegir www-data

RUN a2enmod rewrite

# Save a symlink to the /var/aegir/config/docker.conf file.
RUN mkdir -p /var/aegir/config
RUN mkdir -p /var/aegir/platforms
RUN chown aegir:aegir /var/aegir -R
RUN ln -sf /var/aegir/config/provision.conf /etc/apache2/conf-available/provision.conf
RUN ln -sf /etc/apache2/conf-available/provision.conf /etc/apache2/conf-enabled/provision.conf

COPY sudoers-aegir /etc/sudoers.d/aegir
RUN chmod 0440 /etc/sudoers.d/aegir

# Prepare Aegir Logs folder.
RUN mkdir /var/log/aegir
RUN echo 'Hello, Aegir.' > /var/log/aegir/system.log
RUN chown aegir:aegir /var/log/aegir -R

# Prepare apache foreground script.
COPY httpd-foreground.sh /usr/local/bin/httpd-foreground
RUN chmod +x /usr/local/bin/httpd-foreground

# Prepare Drush & Composer.
RUN wget https://getcomposer.org/download/1.5.5/composer.phar -O /usr/local/bin/composer && chmod +x /usr/local/bin/composer
RUN wget https://github.com/drush-ops/drush/releases/download/8.1.15/drush.phar -O /usr/local/bin/drush && chmod +x /usr/local/bin/drush

USER aegir
WORKDIR /var/aegir

RUN mkdir /var/aegir/config/$AEGIR_SERVER_NAME

VOLUME /var/aegir/config/$AEGIR_SERVER_NAME
VOLUME /var/aegir/platforms

EXPOSE 80
CMD ["httpd-foreground"]