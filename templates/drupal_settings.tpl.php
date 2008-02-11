
  $db_url = '<?php print "$site_db_type://$site_db_username:$site_db_passwd@$site_db_host/$site_db_name"; ?>';
  $profile = "<?php print $site_profile ?>";

  # Additional host wide configuration settings. Useful for safely specifying configuration settings.
  if (file_exists('includes/global.inc')) {
    include_once('includes/global.inc');
  }
