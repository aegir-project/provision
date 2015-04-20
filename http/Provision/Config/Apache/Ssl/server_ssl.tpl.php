NameVirtualHost <?php print "*:" . $http_ssl_port . "\n"; ?>

<IfModule !ssl_module>
  LoadModule ssl_module modules/mod_ssl.so
</IfModule>

<?php include(provision_class_directory('Provision_Config_Apache_Server') . '/server.tpl.php'); ?>
