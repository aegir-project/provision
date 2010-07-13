<?php foreach ($ip_addresses as $ip) : ?>
  NameVirtualHost <?php print $ip . ":" . $http_ssl_port . "\n"; ?>
<?php endforeach; ?>

<IfModule !ssl_module>
  LoadModule ssl_module modules/mod_ssl.so
</IfModule>

<?php include_once(dirname(__FILE__) . '/../apache/server.tpl.php'); ?>
