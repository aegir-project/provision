
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
<?php
    if ($this->redirection) {
      // Redirect all aliases to the main https url.
      print "\n   if (\$host !~ ^({$this->uri})$ ) {\n       rewrite ^/(.*)$  https://{$this->uri}/$1 permanent;\n   }\n";
    }
    if ($server->nginx_has_new_version || $server->nginx_has_upload_progress) {
      print "   include      " . $server->include_path . "/nginx_advanced_include.conf;\n";
    }
    else {
      print "   include      " . $server->include_path . "/nginx_simple_include.conf;\n";
    }
?>
}

<?php endif; ?>

<?php 
   // Generate the standard virtual host too.
   include('http/nginx/vhost.tpl.php');
?>
