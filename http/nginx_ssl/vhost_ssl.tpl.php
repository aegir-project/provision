
<?php if ($this->ssl_enabled && $this->ssl_key) : ?>

<?php
if ($this->redirection) {
  // Redirect all aliases to the main https url using separate vhosts blocks to avoid if{} in Nginx.
  foreach ($this->aliases as $alias_url) {
    print "server {\n";
    print "   listen      {$ip_address}:{$http_ssl_port};\n";
    print "   server_name {$alias_url};\n";
    print "   rewrite ^ \$scheme://{$this->uri}\$request_uri? permanent;\n";
    print "}\n";
  }
}
?>

server {
   include      <?php print "{$server->include_path}"; ?>/fastcgi_ssl_params.conf;
   limit_conn   gulag 10; # like mod_evasive - this allows max 10 simultaneous connections from one IP address
   listen       <?php print "{$ip_address}:{$http_ssl_port}"; ?>;
   server_name  <?php print $this->uri; ?><?php if (!$this->redirection && is_array($this->aliases)) : foreach ($this->aliases as $alias_url) : if (trim($alias_url)) : ?> <?php print $alias_url; ?><?php endif; endforeach; endif; ?>;
   root         <?php print "{$this->root}"; ?>;
   ssl                         on;
   ssl_certificate             <?php print $ssl_cert; ?>;
   ssl_certificate_key         <?php print $ssl_cert_key; ?>;
   ssl_protocols               SSLv3 TLSv1;
   ssl_ciphers                 HIGH:!ADH:!MD5;
   ssl_prefer_server_ciphers   on;
   keepalive_timeout           70;
<?php
$nginx_has_new_version = drush_get_option('nginx_has_new_version');
$nginx_has_upload_progress = drush_get_option('nginx_has_upload_progress');
    if ($nginx_has_new_version || $nginx_has_upload_progress) {
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
