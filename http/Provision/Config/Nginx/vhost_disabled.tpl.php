<?php
$ip_address = !empty($ip_address) ? $ip_address : '*';
?>
server {
  listen       <?php print $ip_address . ':' . $http_port; ?>;
  server_name  <?php print $this->uri . ' ' . implode(' ', $this->aliases); ?>;
  root         /var/www/nginx-default;
  index        index.html index.htm;

  ### Dont't reveal Aegir front-end URL here.
}
