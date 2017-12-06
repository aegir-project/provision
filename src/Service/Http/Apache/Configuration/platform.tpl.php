<Directory <?php print $root; ?>>
    Order allow,deny
    Allow from all
    Satisfy any
    Require all granted

<?php // print $extra_config; ?>


<?php
  if (is_readable("{$root}/.htaccess")) {
    print "\n# Include the platform's htaccess file\n";
    print "Include {$root}/.htaccess\n";
  }
?>

  # Do not read any .htaccess in the platform
  AllowOverride none

</Directory>

