NameVirtualHost <?php print "*:" . $http_ssl_port . "\n"; ?>

<IfModule !ssl_module>
  LoadModule ssl_module modules/mod_ssl.so
</IfModule>

<VirtualHost *:443>
  SSLEngine on
  SSLCertificateFile <?php print $ssl_cert . "\n"; ?>
  SSLCertificateKeyFile <?php print $ssl_cert_key . "\n"; ?>
<?php if (!empty($ssl_chain_cert)) : ?>
  SSLCertificateChainFile <?php print $ssl_chain_cert . "\n"; ?>
<?php endif; ?>
  ServerName default
  Redirect 404 /
</VirtualHost>

<?php include(provision_class_directory('Provision_Config_Apache_Server') . '/server.tpl.php'); ?>
