
  $db_url = '<?php print "$site_db_type://$site_db_user:$site_db_passwd@$site_db_host/$site_db_name"; ?>';
  $profile = "<?php print $site_profile ?>";

  # Additional host wide configuration settings. Useful for safely specifying configuration settings.
  if (file_exists('<?php print PROVISION_CONFIG_PATH . '/' ?>includes/global.inc')) {
    include_once('<?php print PROVISION_CONFIG_PATH . '/' ?>includes/global.inc');
  }
