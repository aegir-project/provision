<?php
$satellite_mode = drush_get_option('satellite_mode');
if (!$satellite_mode && $server->satellite_mode) {
  $satellite_mode = $server->satellite_mode;
}
?>

server {
  listen       *:<?php print $http_port; ?>;
  server_name  <?php print $this->uri . ' ' . implode(' ', str_replace('/', '.', $this->aliases)); ?>;
<?php if ($satellite_mode == 'boa'): ?>
  root         /var/www/nginx-default;
  index        index.html index.htm;
<?php else: ?>
  return       404;
<?php endif; ?>
  ### Do not reveal Aegir front-end URL here.
}
