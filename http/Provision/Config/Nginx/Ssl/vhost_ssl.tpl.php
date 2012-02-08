
<?php if ($this->ssl_enabled && $this->ssl_key) : ?>

<?php if ($this->redirection): ?>
<?php foreach ($this->aliases as $alias_url): ?>
server {
   listen       <?php print "{$ip_address}:{$http_ssl_port}"; ?>;
   server_name  <?php print $alias_url; ?>;
   ssl                         on;
   ssl_certificate             <?php print $ssl_cert; ?>;
   ssl_certificate_key         <?php print $ssl_cert_key; ?>;
   ssl_protocols               SSLv3 TLSv1;
   ssl_ciphers                 HIGH:!ADH:!MD5;
   ssl_prefer_server_ciphers   on;
   keepalive_timeout           70;
   rewrite ^ $scheme://<?php print $this->uri; ?>$request_uri? permanent;
}
<?php endforeach; ?>
<?php endif ?>

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
   <?php print $extra_config; ?>
<?php
$nginx_has_upload_progress = drush_get_option('nginx_has_upload_progress');
    if (drush_drupal_major_version() >= 7 || $this->profile == "hostmaster") {
      if ($server->nginx_has_upload_progress) {
        print "   include      " . $server->include_path . "/nginx_modern_include.conf;\n";
      }
      else {
        print "   include      " . $server->include_path . "/nginx_legacy_include.conf;\n";
      }
    }
    elseif (drush_drupal_major_version() == 5) {
      print "   include      " . $server->include_path . "/nginx_legacy_include.conf;\n";
    }
    else {
      if ($server->nginx_has_upload_progress) {
        print "   include      " . $server->include_path . "/nginx_octopus_include.conf;\n";
      }
      else {
        print "   include      " . $server->include_path . "/nginx_legacy_include.conf;\n";
      }
    }
?>
}

<?php endif; ?>

<?php
   // Generate the standard virtual host too.
   include(provision_class_directory('Provision_Config_Nginx_Site') . '/vhost.tpl.php');
?>
