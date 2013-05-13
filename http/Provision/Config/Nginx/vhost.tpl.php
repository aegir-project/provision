<?php
if ($ssl_redirection || $this->redirection) {
  // Redirect all aliases to the main http url using separate vhosts blocks to avoid if{} in Nginx.
  foreach ($this->aliases as $alias_url) {
    print "server {\n";
    print "  limit_conn   gulag 32;\n";
    print "  listen       *:{$http_port};\n";
    // if we use redirections, we need to change the redirection
    // target to be the original site URL ($this->uri instead of
    // $alias_url)
    if ($this->redirection && $alias_url == $this->redirection) {
      print "  server_name  {$this->uri};\n";
    } else {
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
  fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
  limit_conn    gulag 32; # like mod_evasive - this allows max 32 simultaneous connections from one IP address
  listen        *:<?php print $http_port; ?>;
  server_name   <?php
    // this is the main vhost, so we need to put the redirection
    // target as the hostname (if it exists) and not the original URL
    // ($this->uri)
    if ($this->redirection) {
      print $this->redirection;
    } else {
      print $this->uri;
    }
    if (!$this->redirection && is_array($this->aliases)) {
      foreach ($this->aliases as $alias_url) {
        if (trim($alias_url)) {
          print $alias_url;
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
    print "  include       " . $server->include_path . "/nginx_vhost_common.conf;\n";
  }
}
else {
  print "  include       " . $server->include_path . "/nginx_vhost_common.conf;\n";
}
?>
}
