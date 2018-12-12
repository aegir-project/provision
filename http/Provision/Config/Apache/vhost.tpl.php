<VirtualHost *:<?php print $http_port; ?>>
<?php if ($this->site_mail) : ?>
  ServerAdmin <?php  print $this->site_mail; ?>
<?php endif;?>

<?php
$aegir_root = drush_get_option('aegir_root');
if (!$aegir_root && $server->aegir_root) {
  $aegir_root = $server->aegir_root;
}
?>

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
  foreach ($this->aliases as $alias) {
    print "  ServerAlias " . $alias . "\n";
  }
}
?>

<IfModule mod_rewrite.c>
  RewriteEngine on
<?php
if ($this->redirection || $ssl_redirection) {

  if ($ssl_redirection && !$this->redirection) {
    print " # Redirect aliases in non-ssl to the same alias on ssl.\n";
    print " # Except for /.well-known/acme-challenge/ to prevent potential problems with Let's Encrypt\n";
    print " RewriteCond %{REQUEST_URI} '!/.well-known/acme-challenge/'\n";
    print " RewriteRule ^/*(.*)$ https://%{HTTP_HOST}/$1 [NE,L,R=301]\n";
  }
  elseif ($ssl_redirection && $this->redirection) {
    print " # Redirect all aliases + main uri to the selected alias https uri.\n";
    print " # Except for /.well-known/acme-challenge/ to prevent potential problems with Let's Encrypt\n";
    print " RewriteCond %{REQUEST_URI} '!/.well-known/acme-challenge/'\n";
    print " RewriteRule ^/*(.*)$ https://{$this->redirection}/$1 [NE,L,R=301]\n";
  }
  elseif (!$ssl_redirection && $this->redirection) {
    print " # Redirect all aliases to the selected alias.\n";
    print " RewriteCond %{HTTP_HOST} !^{$this->redirection}$ [NC]\n";
    print " RewriteRule ^/*(.*)$ http://{$this->redirection}/$1 [NE,L,R=301]\n";
  }
}
?>
  RewriteRule ^/files/(.*)$ /sites/<?php print $this->uri; ?>/files/$1 [L]
  RewriteCond <?php print $this->site_path; ?>/files/robots.txt -f
  RewriteRule ^/robots.txt /sites/<?php print $this->uri; ?>/files/robots.txt [L]
</IfModule>

<?php print $extra_config; ?>

    # Error handler for Drupal > 4.6.7
    <Directory ~ "sites/.*/files">
      <Files *>
        SetHandler This_is_a_Drupal_security_line_do_not_remove
      </Files>
      Options None
      Options +FollowSymLinks

      # If we know how to do it safely, disable the PHP engine entirely.
      <IfModule mod_php5.c>
        php_flag engine off
      </IfModule>
    </Directory>

    # Prevent direct reading of files in the private dir.
    # This is for Drupal7 compatibility, which would normally drop
    # a .htaccess in those directories, but we explicitly ignore those
    <Directory ~ "sites/.*/private">
      <Files *>
        SetHandler This_is_a_Drupal_security_line_do_not_remove
      </Files>
      Deny from all
      Options None
      Options +FollowSymLinks

      # If we know how to do it safely, disable the PHP engine entirely.
      <IfModule mod_php5.c>
        php_flag engine off
      </IfModule>
    </Directory>

<?php
$if_subsite = $this->data['http_subdird_path'] . '/' . $this->uri;
if (provision_hosting_feature_enabled('subdirs') && provision_file()->exists($if_subsite)->status()) {
  print "  Include " . $if_subsite . "/*.conf\n";
}
?>

</VirtualHost>
