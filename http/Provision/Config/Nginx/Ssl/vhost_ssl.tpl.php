
<?php if ($this->ssl_enabled && $this->ssl_key) : ?>

<?php
$satellite_mode = drush_get_option('satellite_mode');
if (!$satellite_mode && $server->satellite_mode) {
  $satellite_mode = $server->satellite_mode;
}

$nginx_has_http2 = drush_get_option('nginx_has_http2');
if (!$nginx_has_http2 && $server->nginx_has_http2) {
  $nginx_has_http2 = $server->nginx_has_http2;
}

$aegir_root = d('@server_master')->aegir_root;

if ($nginx_has_http2) {
  $ssl_args = "ssl http2";
}
else {
  $ssl_args = "ssl";
}

if ($satellite_mode == 'boa') {
  $ssl_listen_ip = "*";
}
else {
  $ssl_listen_ip = $ip_address;
}
?>

<?php if ($this->redirection): ?>
<?php foreach ($this->aliases as $alias_url): ?>
server {
  listen       <?php print "{$ssl_listen_ip}:{$http_ssl_port} {$ssl_args}"; ?>;
<?php
  // if we use redirections, we need to change the redirection
  // target to be the original site URL ($this->uri instead of
  // $alias_url)
  if ($this->redirection && $alias_url == $this->redirection) {
    $this->uri = str_replace('/', '.', $this->uri);
    print "  server_name  {$this->uri};\n";
  }
  else {
    $alias_url = str_replace('/', '.', $alias_url);
    print "  server_name  {$alias_url};\n";
  }
?>
  ssl                        on;
<?php if ($satellite_mode == 'boa'): ?>
  ssl_stapling               on;
  ssl_stapling_verify        on;
  resolver 8.8.8.8 8.8.4.4 valid=300s;
  resolver_timeout           5s;
  ssl_dhparam                /etc/ssl/private/nginx-wild-ssl.dhp;
<?php endif; ?>
  ssl_certificate_key        <?php print $ssl_cert_key; ?>;
<?php if (!empty($ssl_chain_cert)) : ?>
  ssl_certificate            <?php print $ssl_chain_cert; ?>;
<?php else: ?>
  ssl_certificate            <?php print $ssl_cert; ?>;
<?php endif; ?>

  ###
  ### Allow access to letsencrypt.org ACME challenges directory.
  ###
  location ^~ /.well-known/acme-challenge {
    alias <?php print $aegir_root; ?>/tools/le/.acme-challenges;
    try_files $uri 404;
  }

  return 301 $scheme://<?php print $this->redirection; ?>$request_uri;
}
<?php endforeach; ?>
<?php endif; ?>

server {
  include       fastcgi_params;
  fastcgi_param MAIN_SITE_NAME <?php print $this->uri; ?>;
  set $main_site_name "<?php print $this->uri; ?>";
  fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
  fastcgi_param HTTPS on;
<?php
  // If any of those parameters is empty for any reason, like after an attempt
  // to import complete platform with sites without importing their databases,
  // it will break Nginx reload and even shutdown all sites on the system on
  // Nginx restart, so we need to use dummy placeholders to avoid affecting
  // other sites on the system if this site is broken.
  if (!$db_type || !$db_name || !$db_user || !$db_passwd || !$db_host) {
    $db_type = 'mysqli';
    $db_name = 'none';
    $db_user = 'none';
    $db_passwd = 'none';
    $db_host = 'localhost';
  }
?>
  fastcgi_param db_type   <?php print urlencode($db_type); ?>;
  fastcgi_param db_name   <?php print urlencode($db_name); ?>;
  fastcgi_param db_user   <?php print urlencode($db_user); ?>;
  fastcgi_param db_passwd <?php print urlencode($db_passwd); ?>;
  fastcgi_param db_host   <?php print urlencode($db_host); ?>;
<?php
  // Until the real source of this problem is fixed elsewhere, we have to
  // use this simple fallback to guarantee that empty db_port does not
  // break Nginx reload which results with downtime for the affected vhosts.
  if (!$db_port) {
    $db_port = $this->server->db_port ? $this->server->db_port : '3306';
  }
?>
  fastcgi_param db_port   <?php print urlencode($db_port); ?>;
  listen        <?php print "{$ssl_listen_ip}:{$http_ssl_port} {$ssl_args}"; ?>;
  server_name   <?php
    // this is the main vhost, so we need to put the redirection
    // target as the hostname (if it exists) and not the original URL
    // ($this->uri)
    if ($this->redirection) {
      print str_replace('/', '.', $this->redirection);
    } else {
      print $this->uri;
    }
    if (!$this->redirection && is_array($this->aliases)) {
      foreach ($this->aliases as $alias_url) {
        if (trim($alias_url)) {
          print " " . str_replace('/', '.', $alias_url);
        }
      }
    } ?>;
  root          <?php print "{$this->root}"; ?>;
  ssl                        on;
<?php if ($satellite_mode == 'boa'): ?>
  ssl_stapling               on;
  ssl_stapling_verify        on;
  resolver 8.8.8.8 8.8.4.4 valid=300s;
  resolver_timeout           5s;
  ssl_dhparam                /etc/ssl/private/nginx-wild-ssl.dhp;
<?php endif; ?>
  ssl_certificate_key        <?php print $ssl_cert_key; ?>;
<?php if (!empty($ssl_chain_cert)) : ?>
  ssl_certificate            <?php print $ssl_chain_cert; ?>;
<?php else: ?>
  ssl_certificate            <?php print $ssl_cert; ?>;
<?php endif; ?>
  <?php print $extra_config; ?>
  include                    <?php print $server->include_path; ?>/nginx_vhost_common.conf;
}

<?php endif; ?>

<?php
  // Generate the standard virtual host too.
  include(provision_class_directory('Provision_Config_Nginx_Site') . '/vhost.tpl.php');
?>
