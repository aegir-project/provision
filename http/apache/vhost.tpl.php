<VirtualHost *:<?php print $http_port; ?>>
<?php if ($this->site_mail) : ?>
  ServerAdmin <?php  print $this->site_mail; ?> 
<?php endif;?>

  DocumentRoot <?php print $this->root; ?> 
    
  ServerName <?php print $this->uri; ?>

  SetEnv db_type  <?php print urlencode($db_type); ?>

  SetEnv db_name  <?php print urlencode($db_name); ?>

  SetEnv db_user  <?php print urlencode($db_user); ?>

  SetEnv db_passwd  <?php print urlencode($db_passwd); ?>

  SetEnv db_host  <?php print urlencode($db_host); ?>

  SetEnv db_port  <?php print urlencode($db_port); ?>


<?php 
if (sizeof($this->aliases)) {
  print "\n ServerAlias " . implode("\n ServerAlias ", $this->aliases) . "\n";

  if ($this->redirection || $ssl_redirection) {
    print "\n RewriteEngine on";

    if ($ssl_redirection) {
      // The URL we want to direct to can't be this virtual host,
      // so we redirect the ServerName too.
      print "\n RewriteCond %{HTTP_HOST} {$this->uri} [OR]";
    }

    print "\n RewriteCond %{HTTP_HOST} " .
      implode(" [OR]\n RewriteCond %{HTTP_HOST} ", $this->aliases) . " [NC]\n";

    if ($ssl_redirection && !$this->redirection) {
      // When we are redirecting for SSL, but not in place of aliases,
      // redirect to the same HTTP host on SSL.
      print " RewriteRule ^/*(.*)$ https://%{HTTP_HOST}/$1 [L,R=301]\n";
    }
    else {
      print " RewriteRule ^/*(.*)$ {$redirect_url}/$1 [L,R=301]\n";
    }
  }
}
?>


<?php print $extra_config; ?>

    # Error handler for Drupal > 4.6.7
    <Directory "<?php print $this->site_path; ?>/files">
      SetHandler This_is_a_Drupal_security_line_do_not_remove
    </Directory>

</VirtualHost>

