FROM provision4/base
USER root
ENV DEBIAN_FRONTEND=noninteractive

RUN (apt-get -qq -o=Dpkg::Use-Pty=0 update && apt-get -qq -o=Dpkg::Use-Pty=0 install\
    apache2 \
    php7.0-common \
    php7.0-cli \
    php7.0-dev \
    php7.0-fpm \
    libpcre3-dev \
    php7.0-gd \
    php7.0-curl \
    php7.0-imap \
    php7.0-json \
    php7.0-opcache \
    php7.0-xml \
    php7.0-mbstring \
    php7.0-mysql \
    php-sqlite3 \
    php-apcu \
    libapache2-mod-php \
    postfix \
    mysql-client \
    ) > /dev/null

RUN a2enmod rewrite

RUN adduser $PROVISION_BASE_USER_NAME www-data
RUN ln -s /var/$PROVISION_BASE_USER_NAME/config/apache.conf /etc/apache2/conf-available/$PROVISION_BASE_USER_NAME.conf
RUN ln -s /etc/apache2/conf-available/$PROVISION_BASE_USER_NAME.conf /etc/apache2/conf-enabled/$PROVISION_BASE_USER_NAME.conf

RUN mkdir -p /var/$PROVISION_BASE_USER_NAME/config
RUN mkdir -p /var/$PROVISION_BASE_USER_NAME/platforms
RUN chown $PROVISION_BASE_USER_NAME:$PROVISION_BASE_USER_NAME /var/$PROVISION_BASE_USER_NAME -R
RUN ls -la /var/$PROVISION_BASE_USER_NAME

COPY sudoers.$PROVISION_BASE_USER_NAME /etc/sudoers.d/$PROVISION_BASE_USER_NAME

COPY entrypoint.sh /usr/local/bin/entrypoint
RUN chmod +x /usr/local/bin/entrypoint

RUN echo "$RUN_PREFIX Creating set-user-ids script..."
COPY set-user-ids.sh /usr/local/bin/set-user-ids
RUN chmod +x /usr/local/bin/set-user-ids

RUN echo "Hello, Provision." > /var/log/$PROVISION_BASE_USER_NAME.log
RUN chown $PROVISION_BASE_USER_NAME:$PROVISION_BASE_USER_NAME /var/log/$PROVISION_BASE_USER_NAME.log -R

ENV SERVER_NAME server_master

ENV COMPOSER_VERSION 1b137f8bf6db3e79a38a5bc45324414a6b1f9df2
RUN echo "$RUN_PREFIX Installing Composer version $COMPOSER_VERSION"
RUN wget https://raw.githubusercontent.com/composer/getcomposer.org/$COMPOSER_VERSION/web/installer -O - -q | php -- --quiet
RUN mv composer.phar /usr/local/bin/composer
RUN chmod +x /usr/local/bin/composer

ENV DRUSH_VERSION 8.1.16
RUN wget https://github.com/drush-ops/drush/releases/download/8.1.16/drush.phar -O - -q > /usr/local/bin/drush
RUN chmod +x /usr/local/bin/drush

USER $PROVISION_BASE_USER_NAME

ENTRYPOINT []
CMD ["entrypoint"]