server {
  listen       <?php print $ip_address . ':' . $http_port; ?>;
  server_name  <?php print $this->uri . ' ' . implode(' ', $this->aliases); ?>;
  root         <?php print $this->root; ?>;
  index        index.php index.html;
  location / {
     root   /var/www/nginx-default;
     index  index.html index.htm;
     rewrite ^/(.*)$  <?php print $this->platform->server->web_disable_url . '/' . $this->uri ?>? permanent;
  }
}
