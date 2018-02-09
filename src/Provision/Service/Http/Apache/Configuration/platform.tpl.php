<Directory <?php print $document_root_full; ?>>
    Order allow,deny
    Allow from all
    Satisfy any
    Require all granted

<?php // print $extra_config; ?>


<?php

// @TODO: This has to be changed, because it's possible $root points to a different folder.
// $root here is /var/aegir/x because it's inside the container.
// If the current user is not running at /var/aegir, $root does not exist.
// So for now, we're adding the include .htaccess directive no matter what.

//  if (is_readable("{$root}/.htaccess")) {
//    print "\n# Include the platform's htaccess file\n";
//    print "Include {$root}/.htaccess\n";
//  }
?>
  # Include the platform's htaccess file
  Include <?php print $document_root_full ?>/.htaccess

  # Do not read any .htaccess in the platform
  AllowOverride none

</Directory>

