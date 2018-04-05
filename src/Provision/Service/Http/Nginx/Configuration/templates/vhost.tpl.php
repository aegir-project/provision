<?php
if ($redirection) {
//  $aegir_root = d('@server_master')->aegir_root;
//  $satellite_mode = d('@server_master')->satellite_mode;
  // Redirect all aliases to the main http url using separate vhosts blocks to avoid if{} in Nginx.
  foreach ($aliases as $alias_url) {
    print "# alias redirection virtual host\n";
    print "server {\n";
    print "  listen       *:{$http_port};\n";
    // if we use redirections, we need to change the redirection
    // target to be the original site URL ($this->uri instead of
    // $alias_url)
    if (isset($redirection_target) && $alias_url == $redirection_target) {
      $uri = str_replace('/', '.', $uri);
      print "  server_name  {$uri};\n";
    } else {
      $alias_url = str_replace('/', '.', $alias_url);
      print "  server_name  {$alias_url};\n";
    }
    print "  access_log   off;\n";
//    if ($satellite_mode == 'boa') {
//      print "\n";
//      print "  ###\n";
//      print "  ### Allow access to letsencrypt.org ACME challenges directory.\n";
//      print "  ###\n";
//      print "  location ^~ /.well-known/acme-challenge {\n";
//      print "    alias {$aegir_root}/tools/le/.acme-challenges;\n";
//      print "    try_files \$uri 404;\n";
//      print "  }\n";
//      print "\n";
//    }
    print "  return 301 \$scheme://{$redirection_target}\$request_uri;\n";
    print "}\n";
  }
}
?>

server {
  include       fastcgi_params;

  # Block https://httpoxy.org/ attacks.
  fastcgi_param HTTP_PROXY "";

  fastcgi_param MAIN_SITE_NAME <?php print $uri; ?>;
  set $main_site_name "<?php print $uri; ?>";
  fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
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
//  if (!$db_port) {
//    $db_port = $this->server->db_port ? $this->server->db_port : '3306';
//  }
?>
  fastcgi_param db_port   <?php print urlencode($db_port); ?>;
  listen        *:<?php print $http_port; ?>;
  server_name   <?php
    // this is the main vhost, so we need to put the redirection
    // target as the hostname (if it exists) and not the original URL
    // ($uri)
    if ($redirection) {
      print str_replace('/', '.', $redirection);
    } else {
      print $uri;
    }
    if (!$redirection && is_array($aliases)) {
      foreach ($aliases as $alias_url) {
        if (trim($alias_url)) {
          print " " . str_replace('/', '.', $alias_url);
        }
      }
    } ?>;
  root          <?php print "{$document_root_full}"; ?>;
  <?php print $extra_config; ?>
<?php
if ($redirection || $ssl_redirection) {
  if ($ssl_redirection && !$redirection) {
    // redirect aliases in non-ssl to the same alias on ssl.
    print "\n  return 301 https://\$host\$request_uri;\n";
  }
  elseif ($ssl_redirection && $redirection) {
    // redirect all aliases + main uri to the main https uri.
    print "\n  return 301 https://{$redirection}\$request_uri;\n";
  }
  elseif (!$ssl_redirection && $redirection) {
    // Commenting out until we fix where include_path is coming from.
    //    print "  include       " . $server->include_path . "/nginx_vhost_common.conf;\n";
  }
}
else {
  print "  include       " . $server_config_path . "/nginx_vhost_common.conf;\n";
}
//$if_subsite = $this->data['http_subdird_path'] . '/' . $uri;
//if (provision_hosting_feature_enabled('subdirs') && provision_file()->exists($if_subsite)->status()) {
//  print "  include       " . $if_subsite . "/*.conf;\n";
//}
?>
}
