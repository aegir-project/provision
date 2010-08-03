server {
  listen       <?php print $ip_address . ':' . $http_port; ?>;
  server_name <?php if ($this->redirection && is_array($this->aliases)) : foreach ($this->aliases as $alias_url) : if (trim($alias_url)) : ?> <?php print $alias_url; ?><?php endif; endforeach; endif; ?>;
  root         <?php print $this->root; ?>;
  index        index.php index.html;
  location / {
     root   /var/www/nginx-default;
     index  index.html index.htm;
     <?php if ($ssl_redirect): ?>
     rewrite ^/(.*)$  https://<?php print $this->uri ?>/$1 permanent;
     <?php else: ?>
     rewrite ^/(.*)$  http://<?php print $this->uri ?>/$1 permanent;
     <?php endif; ?>
  }
}
