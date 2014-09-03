server {
  listen       *:<?php print $http_port; ?>;
  server_name  <?php print $this->uri . ' ' . implode(' ', $this->aliases); ?>;
  root         /var/www/nginx-default;
  index        index.html index.htm;
  ### Do not reveal Aegir front-end URL here.
}
