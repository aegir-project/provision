
<?php if ($this->ssl_enabled && $this->ssl_key) : ?>

server {
<?php 
   print "   include      " . $server->include_path . "/fastcgi_ssl_params.conf;\n";
?>
   limit_conn   gulag 10; # like mod_evasive - this allows max 10 simultaneous connections from one IP address
   listen       <?php print "{$ip_address}:{$http_ssl_port}"; ?>;
   server_name  <?php print $this->uri . ' ' . implode(' ', $this->aliases); ?>;
   root         <?php print $this->root; ?>;
   index        index.php index.html;
   ssl                         on;
   ssl_certificate             <?php print $ssl_cert; ?>;
   ssl_certificate_key         <?php print $ssl_cert_key; ?>;
   ssl_protocols               SSLv2 SSLv3 TLSv1;
   ssl_ciphers                 ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+LOW:+SSLv2:+EXP;
   ssl_prefer_server_ciphers   on;
   keepalive_timeout           70;

  location / {
     root   /var/www/nginx-default;
     index  index.html index.htm;
     rewrite ^/(.*)$  <?php print $this->platform->server->web_disable_url . '/' . $this->uri ?>? permanent;
  }

}

<?php endif; ?>

<?php 
   // Generate the standard virtual host too.
   include('http/nginx/vhost_disabled.tpl.php');
?>
