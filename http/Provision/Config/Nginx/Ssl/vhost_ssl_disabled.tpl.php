
<?php if ($this->ssl_enabled && $this->ssl_key) : ?>

<?php
$satellite_mode = drush_get_option('satellite_mode');
if (!$satellite_mode && $server->satellite_mode) {
  $satellite_mode = $server->satellite_mode;
}
?>

server {
  listen       <?php print "{$ip_address}:{$http_ssl_port}"; ?>;
  server_name  <?php print $this->uri . ' ' . implode(' ', str_replace('/', '.', $this->aliases)); ?>;
<?php if ($satellite_mode == 'boa'): ?>
  root         /var/www/nginx-default;
  index        index.html index.htm;
<?php else: ?>
  return       404;
<?php endif; ?>
  ssl                        on;
  ssl_certificate            <?php print $ssl_cert; ?>;
  ssl_certificate_key        <?php print $ssl_cert_key; ?>;
  ssl_protocols              SSLv3 TLSv1 TLSv1.1 TLSv1.2;
  ssl_ciphers                RC4:HIGH:!aNULL:!MD5;
  ssl_prefer_server_ciphers  on;
  keepalive_timeout          70;
  ### Do not reveal Aegir front-end URL here.
}

<?php endif; ?>

<?php
  // Generate the standard virtual host too.
  include(provision_class_directory('Provision_Config_Nginx_Site') . '/vhost_disabled.tpl.php');
?>
