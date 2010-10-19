<Directory <?php print $this->root; ?>>
    Order allow,deny
    Allow from all
<?php print $extra_config; ?>

<?php
  if (file_exists("{$this->root}/.htaccess")) {
    print "\n# Include the platform's htaccess file\n";
    print "Include {$this->root}/.htaccess\n";
  }
?>

  # Do not read the platform's .htaccess
  AllowOverride none

  <IfModule mod_rewrite.c>
    RewriteEngine on
    # allow files to be accessed without /sites/fqdn/
    RewriteRule ^files/(.*)$ /sites/%{HTTP_HOST}/files/$1 [L]
  </IfModule>
</Directory>

