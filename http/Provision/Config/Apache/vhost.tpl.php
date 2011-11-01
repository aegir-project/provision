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
}
 
if ($this->redirection || $ssl_redirection) {
  print " RewriteEngine on\n";

  if ($ssl_redirection && !$this->redirection) {
    // redirect aliases in non-ssl to the same alias on ssl.
    print " RewriteRule ^/*(.*)$ https://%{HTTP_HOST}/$1 [L,R=301]\n";
  }
  elseif ($ssl_redirection && $this->redirection) {
    // redirect all aliases + main uri to the main https uri.
    print " RewriteRule ^/*(.*)$ https://{$this->uri}/$1 [L,R=301]\n";
  }
  elseif (!$ssl_redirection && $this->redirection) {
    // Redirect all aliases to the main http url.
    print " RewriteCond %{HTTP_HOST} !^{$this->uri}$ [NC]\n";
    print " RewriteRule ^/*(.*)$ http://{$this->uri}/$1 [L,R=301]\n";
  }
}
?>


<?php print $extra_config; ?>

    # Error handler for Drupal > 4.6.7
    <Directory "<?php print $this->site_path; ?>/files">
      SetHandler This_is_a_Drupal_security_line_do_not_remove
    </Directory>

    # Prevent direct reading of files in the private dir.
    # This is for Drupal7 compatibility, which would normally drop
    # a .htaccess in those directories, but we explicitly ignore those
    <DirectoryMatch "<?php print $this->site_path; ?>/private/(files|temp)/" >
       SetHandler This_is_a_Drupal_security_line_do_not_remove
       Deny from all
       Options None
       Options +FollowSymLinks
    </DirectoryMatch>
    

</VirtualHost>

