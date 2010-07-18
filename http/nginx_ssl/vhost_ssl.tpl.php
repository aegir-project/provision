
<?php if ($this->ssl_enabled && $this->ssl_key) : ?>

server {
   limit_conn   gulag 10; # like mod_evasive - this allows max 10 simultaneous connections from one IP address
   listen       <?php print "{$ip_address}:{$http_ssl_port}"; ?>;
   server_name  <?php print $this->uri; ?> <?php if (!$this->redirection && is_array($this->aliases)) : foreach ($this->aliases as $alias_url) : if (trim($alias_url)) : ?> <?php $alias_url = "." . $alias_url; ?> <?php print $alias_url; ?> <?php endif; endforeach; endif; ?>;
   root         <?php print $this->root; ?>;
   index        index.php index.html;
   ssl                         on;
   ssl_certificate             <?php print $ssl_cert; ?>;
   ssl_certificate_key         <?php print $ssl_cert_key; ?>;
   ssl_session_timeout         5m;
   ssl_protocols               SSLv2 SSLv3 TLSv1;
   ssl_ciphers                 ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+LOW:+SSLv2:+EXP;
   ssl_prefer_server_ciphers   on;
   include      <?php print $server->include_path ?>/nginx_include.conf;
}

<?php endif; ?>

<?php 
   if ($this->ssl_enabled != 2) :
     // Generate the standard virtual host too.
     include('http/nginx/vhost.tpl.php');

   else :
     // Generate a virtual host that redirects all HTTP traffic to https.
?>

server {
  listen       <?php print $ip_address . ':' . $http_port; ?>;
  server_name  <?php print $this->uri; ?> <?php if (!$this->redirection && is_array($this->aliases)) : foreach ($this->aliases as $alias_url) : if (trim($alias_url)) : ?> <?php $alias_url = "." . $alias_url; ?> <?php print $alias_url; ?> <?php endif; endforeach; endif; ?>;
  root         <?php print $this->root; ?>;
  index        index.php index.html;

  location / {
     root   /var/www/nginx-default;
     index  index.html index.htm;
     rewrite ^/(.*)$  <?php print $ssl_redirect_url ?>/$1 permanent;
  }

  error_page   500 502 503 504  /50x.html;
  location = /50x.html {
     root   /var/www/nginx-default;
  }

}

<?php endif; ?>
