server {
   limit_conn   gulag 10; # like mod_evasive - this allows max 10 simultaneous connections from one IP address
   listen       <?php print $ip_address . ':' . $http_port; ?>;
   server_name  <?php print $this->uri; ?><?php if (!$this->redirection && is_array($this->aliases)) : foreach ($this->aliases as $alias_url) : if (trim($alias_url)) : ?> <?php print $alias_url; ?><?php endif; endforeach; endif; ?>;
   root         <?php print $this->root; ?>;
   index        index.php index.html;
<?php 
    $this->server->shell_exec('nginx -V');
    if (preg_match("/nginx\/0\.8\./", implode('', drush_shell_exec_output()), $match)) {
      print '   include      ' . $server->include_path . '/nginx_advanced_include.conf';
    }
    elseif (preg_match("/(nginx-upload-progress-module)/", implode('', drush_shell_exec_output()), $match)) {
      print '   include      ' . $server->include_path . '/nginx_advanced_include.conf';
    }
    else {
      print '   include      ' . $server->include_path . '/nginx_simple_include.conf';
    }
?>;
}

<?php
if ($this->redirection) {
  require(dirname(__FILE__) . '/http/nginx/vhost_redirect.tpl.php');
}
