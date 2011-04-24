<?php

/**
 * Provision verify command
 *
 * Disable an enabled and installed site
 */
function drush_provision_drupal_provision_verify_validate() {
  if (d()->type === 'site') {
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
    provision_prepare_environment();

    _provision_drupal_create_settings_file();
    drush_errors_on();
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
  }

}


/**
 * implementation of provision_verify
 */
function drush_provision_drupal_pre_provision_verify() {
  if (d()->type === 'server') {

    $config_root = dirname(d()->config_path);
    provision_file()->create_dir($config_root, dt('Provision configuration root'), 0711);
    provision_file()->create_dir(d()->config_path, dt('Provision configuration'), 0711);
    provision_file()->create_dir(d()->clients_path, dt('Provision client home'), 0711);
    if (d()->name == '@server_master') {
      provision_file()->create_dir(d()->backup_path, dt('Backup'), 0700);
      provision_file()->create_dir(d()->include_path, dt('Provision PHP configuration'), 0711);
      if (!provision_file()->exists(d()->include_path . '/global.inc')->succeed('Global configuration file exists')->status()) {
        // Create an empty global.inc so the include doesn't fail with
        // open_basedir restrictions
        $config = new provisionConfig_global_settings(d()->name);
        $config->write();
      }
    }
    else {
      if (provision_file()->exists(d()->include_path)) {
        d()->service('http')->sync(d()->include_path);
      }
    }
  }
  elseif (d()->type === 'platform') {
    // create a platform from a makefile, if it doesnt already exist and the makefile does.
    if (!provision_file()->exists(d()->root)->status() && !empty(d()->makefile)) {
      drush_log(dt("Platform path does not exist, fetching from drush makefile"));
      $options = array('root' => null, 'uri' => null);
      // propagate working-copy args downward
      if (drush_get_option('working-copy')) {
        $options['working-copy'] = 1;
      }
      $options[] = d()->makefile;
      $options[] = d()->root;
      drush_backend_invoke("make", $options);
      if (drush_get_error()) {
        return drush_set_error("DRUSH_MAKE_FAILED", 
          "Could not download platform using drush make. No platform present");
      }
    }

    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_ROOT);
    provision_file()->writable(d()->root . '/sites')
      ->succeed('Drupal sites directory @path is writable by the provisioning script')
      ->fail('Drupal sites directory @path is not writable by the provisioning script', 'PROVISION_SITES_DIR_NOT_WRITABLE');

    drush_set_option('sites', array_keys((array) provision_drupal_find_sites()), 'drupal'); 
    drush_log(dt("This platform is running @short_name @version", array('@short_name' => 'drupal', '@version' => VERSION)));
    drush_set_option('packages', _scrub_object(provision_find_packages()), 'drupal');
  }
  elseif (d()->type === 'site') {
    drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
    // This is the actual drupal provisioning requirements. 
    _provision_drupal_create_directories();
    _provision_drupal_maintain_aliases();
    // Requires at least the database settings to complete.
   _provision_drupal_create_settings_file();
  }
}

class provisionConfig_global_settings extends provisionConfig {
  public $template = 'global_settings.tpl.php';
  public $description = 'Global settings.php file';

  function filename() {
    return $this->include_path . '/global.inc';
  }
}

/**
 * Implementation of hook_provision_post_verify
 */
function drush_provision_drupal_post_provision_verify() {
  if (d()->type === 'site') {
    _provision_drupal_rebuild_caches();
    drush_set_option('installed', TRUE, 'site');

    _provision_client_create_symlink();
    drush_include_engine('drupal', 'cron_key');
  }
}

