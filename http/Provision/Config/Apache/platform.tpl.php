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

  # Do not read any .htaccess in the platform
  AllowOverride none

</Directory>

