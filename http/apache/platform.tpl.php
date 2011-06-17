<Directory <?php print $this->root; ?>>
    Order allow,deny
    Allow from all
<?php print $extra_config; ?>

  <IfModule mod_rewrite.c>
    RewriteEngine on
    # allow files to be accessed without /sites/fqdn/
    RewriteRule ^files/(.*)$ /sites/%{HTTP_HOST}/files/$1 [L]

    RewriteCond <?php print $this->root; ?>/sites/%{HTTP_HOST}/files/robots.txt -f
    RewriteRule ^robots.txt /sites/%{HTTP_HOST}/files/robots.txt [L]
  </IfModule>

<?php
  if (file_exists("{$this->root}/.htaccess")) {
    print "\n# Include the platform's htaccess file\n";
    print "Include {$this->root}/.htaccess\n";
  }
?>

  # Do not read any .htaccess in the platform
  AllowOverride none

</Directory>

