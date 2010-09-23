server {
<?php 
   print "   include      " . $server->include_path . "/fastcgi_params.conf;\n";
?>
   limit_conn   gulag 10; # like mod_evasive - this allows max 10 simultaneous connections from one IP address
   listen       <?php print $ip_address . ':' . $http_port; ?>;
   server_name  <?php print $this->uri . ' ' . implode(' ', $this->aliases); ?>;
   root         <?php print $this->root; ?>;
   index        index.php index.html;
<?php
if ($this->redirection || $ssl_redirection) {
  if ($ssl_redirection && !$this->redirection) {
    // redirect aliases in non-ssl to the same alias on ssl.
    print "\n   rewrite ^/(.*)$  https://\$host/$1 permanent;\n";
  }
  elseif ($ssl_redirection && $this->redirection) {
    // redirect all aliases + main uri to the main https uri.
    print "\n   rewrite ^/(.*)$  https://{$this->uri}/$1 permanent;\n";
  }
  elseif (!$ssl_redirection && $this->redirection) {
    // Redirect all aliases to the main http url.
    print "\n   if (\$host !~ ^({$this->uri})$ ) {\n       rewrite ^/(.*)$  http://{$this->uri}/$1 permanent;\n   }\n";
    if ($server->nginx_has_new_version || $server->nginx_has_upload_progress) {
      print "   include      " . $server->include_path . "/nginx_advanced_include.conf;\n";
    }
    else {
      print "   include      " . $server->include_path . "/nginx_simple_include.conf;\n";
    }
  }
}
else {
  if ($server->nginx_has_new_version || $server->nginx_has_upload_progress) {
    print "   include      " . $server->include_path . "/nginx_advanced_include.conf;\n";
  }
  else {
    print "   include      " . $server->include_path . "/nginx_simple_include.conf;\n";
  }
}
?>
}
