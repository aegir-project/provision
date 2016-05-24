
<?php if ($this->ssl_enabled && $this->ssl_key) : ?>

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
else {
  $ssl_listen_ip = $ip_address;
}
?>

server {
  listen       <?php print "{$ssl_listen_ip}:{$http_ssl_port} {$ssl_args}"; ?>;
  server_name  <?php print $this->uri . ' ' . implode(' ', str_replace('/', '.', $this->aliases)); ?>;
<?php if ($satellite_mode == 'boa'): ?>
  root         /var/www/nginx-default;
  index        index.html index.htm;
  ### Do not reveal Aegir front-end URL here.
<?php else: ?>
  return 302 <?php print $this->platform->server->web_disable_url . '/' . $this->uri ?>;
<?php endif; ?>
  ssl                        on;
<?php if ($satellite_mode == 'boa'): ?>
  ssl_stapling               on;
  ssl_stapling_verify        on;
  resolver 8.8.8.8 8.8.4.4 valid=300s;
  resolver_timeout           5s;
  ssl_dhparam                /etc/ssl/private/nginx-wild-ssl.dhp;
<?php endif; ?>
  ssl_certificate_key        <?php print $ssl_cert_key; ?>;
<?php if (!empty($ssl_chain_cert)) : ?>
  ssl_certificate            <?php print $ssl_chain_cert; ?>;
<?php else: ?>
  ssl_certificate            <?php print $ssl_cert; ?>;
<?php endif; ?>
}

<?php endif; ?>

<?php
  // Generate the standard virtual host too.
  include(provision_class_directory('Provision_Config_Nginx_Site') . '/vhost_disabled.tpl.php');
?>
