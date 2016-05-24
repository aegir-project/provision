<?php include(provision_class_directory('Provision_Config_Nginx_Server') . '/server.tpl.php'); ?>

#######################################################
###  nginx default ssl server
#######################################################

<?php
$satellite_mode = drush_get_option('satellite_mode');
if (!$satellite_mode && $server->satellite_mode) {
  $satellite_mode = $server->satellite_mode;
}

$nginx_has_http2 = drush_get_option('nginx_has_http2');
if (!$nginx_has_http2 && $server->nginx_has_http2) {
  $nginx_has_http2 = $server->nginx_has_http2;
}

if ($nginx_has_http2) {
  $ssl_args = "ssl http2";
}
else {
  $ssl_args = "ssl";
}

if ($satellite_mode == 'boa') {
  $ssl_listen_ip = "*";
}
?>

server {
<?php if ($satellite_mode == 'boa'): ?>
  listen       <?php print "{$ssl_listen_ip}:{$http_ssl_port} {$ssl_args}"; ?>;
<?php else: ?>
<?php foreach ($server->ip_addresses as $ip) :?>
  listen       <?php print "{$ip}:{$http_ssl_port} {$ssl_args}"; ?>;
<?php endforeach; ?>
<?php endif; ?>
  server_name  _;
  location / {
<?php if ($satellite_mode == 'boa'): ?>
    root   /var/www/nginx-default;
    index  index.html index.htm;
<?php else: ?>
    return 404;
<?php endif; ?>
  }
}
