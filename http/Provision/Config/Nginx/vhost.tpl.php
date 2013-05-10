<?php
$ip_address = !empty($ip_address) ? $ip_address : '*';
if ($ssl_redirection || $this->redirection) {
  // Redirect all aliases to the main http url using separate vhosts blocks to avoid if{} in Nginx.
  foreach ($this->aliases as $alias_url) {
    print "server {\n";
    print "  limit_conn   gulag 32;\n";
    if ($ip_address == '*') {
      print "  listen       {$ip_address}:{$http_port};\n";
    }
    else {
      foreach ($server->ip_addresses as $ip) {
        print "  listen       {$ip}:{$http_port};\n";
      }
    }
    print "  server_name  {$alias_url};\n";
    print "  access_log   off;\n";
    print "  rewrite ^ \$scheme://{$this->uri}\$request_uri? permanent;\n";
    print "}\n";
  }
}
?>

server {
  limit_conn   gulag 32; # like mod_evasive - this allows max 32 simultaneous connections from one IP address
<?php
if ($ip_address == '*') {
  print "  listen       {$ip_address}:{$http_port};\n";
}
else {
  foreach ($server->ip_addresses as $ip) {
    print "  listen       {$ip}:{$http_port};\n";
  }
}
?>
  server_name  <?php print $this->uri; ?><?php if (!$this->redirection && is_array($this->aliases)) : foreach ($this->aliases as $alias_url) : if (trim($alias_url)) : ?> <?php print $alias_url; ?><?php endif; endforeach; endif; ?>;
  root         <?php print "{$this->root}"; ?>;
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
    if ($server->extended_nginx_config) {
      print "  include      " . $server->include_path . "/nginx_advanced_include.conf;\n";
    }
    else {
      print "  include      " . $server->include_path . "/nginx_simple_include.conf;\n";
    }
  }
}
else {
  if ($server->extended_nginx_config) {
    print "  include      " . $server->include_path . "/nginx_advanced_include.conf;\n";
  }
  else {
    print "  include      " . $server->include_path . "/nginx_simple_include.conf;\n";
  }
}
?>
}
