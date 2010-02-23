<?php

/**
 * Drupal-based server configuration
 */
function drush_provision_drupal_provision_server() {
  if (!provision_path('exists', drush_get_option('config_path') .  '/includes/global.inc', TRUE, dt("Global configuration file exists"))) {
    # create an empty global.inc so the include doesn't fail with
    # open_basedir restrictions
    if (!$file = fopen(drush_get_option('config_path') .  '/includes/global.inc', 'a')) {
      drush_set_error('PROVISION_FRAMEWORK_ERROR', dt('Cannot create global settings configuration'));
    } else {
      fwrite($file, "<?php # global settings.php");
      fclose($file);
    }
  }
  _provision_create_dir(drush_get_option('backup_path'), dt('Backup'), 0700);
  provision_path("writable", drush_get_option('sites_path'), TRUE, dt("Drupal sites directory is writable by the provisioning script"),
                 dt("Drupal sites directory is not writable by the provisioning script"), 'PROVISION_SITES_DIR_NOT_WRITABLE');
}
