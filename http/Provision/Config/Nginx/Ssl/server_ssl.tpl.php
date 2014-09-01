<?php include(provision_class_directory('Provision_Config_Nginx_Server') . '/server.tpl.php'); ?>

#######################################################
###  nginx default ssl server
#######################################################

server {
<?php foreach ($server->ip_addresses as $ip) :?>
  listen       <?php print $ip . ':' . $http_ssl_port; ?>;
<?php endforeach; ?>
  server_name  _;
  location / {
    root   /var/www/nginx-default;
    index  index.html index.htm;
  }
}
