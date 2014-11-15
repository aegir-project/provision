<?php
if ($ssl_redirection || $this->redirection) {
  // Redirect all aliases to the main http url using separate vhosts blocks to avoid if{} in Nginx.
  foreach ($this->aliases as $alias_url) {
    print "# alias redirection virtual host\n";
    print "server {\n";
    print "  listen       *:{$http_port};\n";
    // if we use redirections, we need to change the redirection
    // target to be the original site URL ($this->uri instead of
    // $alias_url)
    if ($this->redirection && $alias_url == $this->redirection) {
      $this->uri = str_replace('/', '.', $this->uri);
      print "  server_name  {$this->uri};\n";
    } else {
      $alias_url = str_replace('/', '.', $alias_url);
      print "  server_name  {$alias_url};\n";
    }
    print "  access_log   off;\n";
    print "  rewrite ^ \$scheme://{$this->redirection}\$request_uri? permanent;\n";
    print "}\n";
  }
}
?>

server {
  include       fastcgi_params;
  fastcgi_param MAIN_SITE_NAME <?php print $this->uri; ?>;
  set $main_site_name "<?php print $this->uri; ?>";
  fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
  fastcgi_param db_type   <?php print urlencode($db_type); ?>;
  fastcgi_param db_name   <?php print urlencode($db_name); ?>;
  fastcgi_param db_user   <?php print urlencode($db_user); ?>;
  fastcgi_param db_passwd <?php print urlencode($db_passwd); ?>;
  fastcgi_param db_host   <?php print urlencode($db_host); ?>;
  fastcgi_param db_port   <?php print urlencode($db_port); ?>;
  listen        *:<?php print $http_port; ?>;
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
  <?php print $extra_config; ?>
<?php
if ($this->redirection || $ssl_redirection) {
  if ($ssl_redirection && !$this->redirection) {
    // redirect aliases in non-ssl to the same alias on ssl.
    print "\n  rewrite ^ https://\$host\$request_uri? permanent;\n";
  }
  elseif ($ssl_redirection && $this->redirection) {
    // redirect all aliases + main uri to the main https uri.
    print "\n  rewrite ^ https://{$this->uri}\$request_uri? permanent;\n";
  }
  elseif (!$ssl_redirection && $this->redirection) {
    // Redirect all aliases to the main http url.
    print "\n  rewrite ^ http://{$this->redirection}\$request_uri? permanent;\n";
  }
}
else {
  print "  include       " . $server->include_path . "/nginx_vhost_common.conf;\n";
}
$if_subsite = $this->data['http_subdird_path'] . '/' . $this->uri;
if (provision_hosting_feature_enabled('subdirs') && provision_file()->exists($if_subsite)->status()) {
  print "  include       " . $if_subsite . "/*.conf;\n";
}
?>
}
