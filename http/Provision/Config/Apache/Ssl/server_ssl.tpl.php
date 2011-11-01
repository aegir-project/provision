<?php foreach ($server->ip_addresses as $ip) : ?>
  NameVirtualHost <?php print $ip . ":" . $http_ssl_port . "\n"; ?>
<?php endforeach; ?>

<IfModule !ssl_module>
  LoadModule ssl_module modules/mod_ssl.so
</IfModule>

<?php include('http/apache/server.tpl.php'); ?>
