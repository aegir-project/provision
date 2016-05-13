
<?php if ($this->ssl_enabled && $this->ssl_key) : ?>

<?php
$satellite_mode = drush_get_option('satellite_mode');
if (!$satellite_mode && $server->satellite_mode) {
  $satellite_mode = $server->satellite_mode;
}
?>

server {
  listen       <?php print "*:{$http_ssl_port} ssl http2"; ?>;
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
  keepalive_timeout          70;
  ### Do not reveal Aegir front-end URL here.
}

<?php endif; ?>

<?php
  // Generate the standard virtual host too.
  include(provision_class_directory('Provision_Config_Nginx_Site') . '/vhost_disabled.tpl.php');
?>
