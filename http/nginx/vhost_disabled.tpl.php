server {
  listen       <?php print $ip_address . ':' . $http_port; ?>;
  server_name  <?php print $this->uri; ?><?php if (is_array($this->aliases)) : foreach ($this->aliases as $alias_url) : if (trim($alias_url)) : ?> <?php print $alias_url; ?><?php endif; endforeach; endif; ?>;
  root         <?php print $this->root; ?>;
  index        index.php index.html;

  location / {
     root   /var/www/nginx-default;
     index  index.html index.htm;
     rewrite ^/(.*)$  <?php print $this->platform->server->web_disable_url . '/' . $this->uri ?>? permanent;
  }
}
