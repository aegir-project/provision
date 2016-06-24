<?php

/**
 * @file
 * Code related to verify tasks.
 */

/**
 * Provision verify command.
 *
 * Disable an enabled and installed site
 */
function drush_provision_drupal_provision_verify_validate() {
  if (d()->type === 'site') {
    d()->service('db')->connect();

    // START from drush_provision_drupal_provision_install_validate
    if (!d()->uri) {
      return drush_set_error("PROVISION_URL_REQUIRED", dt("You need to specify a valid url to install a site"));
    }
    /** From install hook, we obviously don't want to run it.
    if (_provision_drupal_site_exists()) {
      return drush_set_error('PROVISION_SITE_INSTALLED');
    }
     */
    // END from drush_provision_drupal_provision_install_validate

    // START original
//    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
//    provision_prepare_environment();
//
//    _provision_drupal_create_settings_file();
//    provision_drupal_push_site();
//
//    drush_errors_on();
//    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
    // END original
  }
}


/**
 * Implements drush_hook_pre_COMMAND().
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
        // open_basedir restrictions.
        $config = new Provision_Config_Global_Settings(d()->name);
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
    // Create a platform from a makefile, if it doesnt already exist and the makefile does.
    if (!provision_file()->exists(d()->root)->status() && !empty(d()->makefile)) {
      drush_log(dt("Platform path does not exist, fetching from drush makefile"));
      $arguments = array();
      $options = array();
      // Propagate working-copy args downward.
      if (drush_get_option('working-copy') || !empty(d()->make_working_copy)) {
        $options[] = '--working-copy';
      }
      $arguments[] = d()->makefile;
      $arguments[] = d()->root;
      drush_invoke_process('@none', "make", $arguments, $options);
      if (drush_get_error()) {
        return drush_set_error("DRUSH_MAKE_FAILED",
          "Could not download platform using drush make. No platform present");
      }
    }

    // Re-set the ROOT PATH for Drush.
    // In cases where we just build the platform Drush would have cleared this value.
    // See Drush commit 3b74d40b1228f022464b92243c16127eb613c2df
    drush_set_context('DRUSH_SELECTED_DRUPAL_ROOT', d()->root);

    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_ROOT);
    // If we couldn't bootstrap, there's no sense in continuing past this point.
    if (drush_get_error()) {
      return drush_set_error("DRUSH_INVALID_DRUPAL_ROOT",
        "Drush could not bootstrap this platform. Please check the platform directory exists and is readable.");
    }
    provision_file()->writable(d()->root . '/sites')
      ->succeed('Drupal sites directory @path is writable by the provisioning script')
      ->fail('Drupal sites directory @path is not writable by the provisioning script', 'PROVISION_SITES_DIR_NOT_WRITABLE');

    // Ensure sites.php file exists for Drupal 8 and above.
    $sitesfile = "sites/sites.php";
    if (drush_drupal_major_version() >= 8 && !file_exists($sitesfile)) {
      if (!drush_op('copy', 'sites/example.sites.php', $sitesfile) && !drush_get_context('DRUSH_SIMULATE')) {
        return drush_set_error(dt('Failed to copy sites/sites.php to @sitesfile', array('@sitesfile' => $sitesfile)));
      }
    }

    drush_set_option('sites', array_keys((array) provision_drupal_find_sites()), 'drupal');
    drush_log(dt("This platform is running @short_name @version", array('@short_name' => 'drupal', '@version' => drush_drupal_version())));
    drush_set_option('packages', _scrub_object(provision_find_packages()), 'drupal');
    provision_drupal_push_site();
  }
  elseif (d()->type === 'site') {
    _provision_drupal_create_directories();
    d()->service('db')->create_site_database();



    // START original
//    drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
//    // This is the actual drupal provisioning requirements.
//    _provision_drupal_create_directories();
//    _provision_drupal_maintain_aliases();
//    _provision_drupal_ensure_htaccess_update();
//    // Requires at least the database settings to complete.
//
//    _provision_drupal_create_settings_file();
//
//    // If this is the hostmaster site, save the ~/.drush/drushrc.php file.
//    if (d()->root == d('@hostmaster')->root && d()->uri == d('@hostmaster')->uri) {
//      $aegir_drushrc = new Provision_Config_Drushrc_Aegir();
//      $aegir_drushrc->write();
//    }
//
//    provision_drupal_push_site(drush_get_option('override_slave_authority', FALSE));
    // END original
  }
}

/**
 * Verify command hook
 */
function drush_provision_drupal_provision_verify() {


  // START from drush_provision_drupal_provision_install
  // Requires at least the database settings to complete.
  provision_prepare_environment();
  _provision_drupal_create_settings_file();

  $config = new Provision_Config_Drushrc_Site(d()->name, drush_get_context('site'));
  $config->write();
//  provision_drupal_push_site($override_slave_authority = TRUE);

  provision_save_site_data();

  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

//  // call a backend task to do the actual installation.
//  $result = provision_backend_invoke(d()->name, "provision-install-backend", array(), array('client_email' => drush_get_option('client_email')));
//  // pass the login link back to the front end.
//  drush_set_option('login_link', $result['context']['login_link']);
//
//  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
//  drush_set_option('installed', TRUE, 'site');
  _provision_drupal_maintain_aliases();

  // END from drush_provision_drupal_provision_install

  // START from drush_provision_drupal_pre_provision_verify()
  _provision_drupal_ensure_htaccess_update();

  // If this is the hostmaster site, save the ~/.drush/drushrc.php file.
  if (d()->root == d('@hostmaster')->root && d()->uri == d('@hostmaster')->uri) {
    $aegir_drushrc = new Provision_Config_Drushrc_Aegir();
    $aegir_drushrc->write();
  }

  provision_drupal_push_site(drush_get_option('override_slave_authority', FALSE));
  // END from drush_provision_drupal_pre_provision_verify()

}

/**
 * Implements drush_hook_provision_post_COMMAND().
 */
function drush_provision_drupal_post_provision_verify() {
  if (d()->type === 'site') {

    if (drush_get_option('installed', FALSE)) {
      _provision_drupal_rebuild_caches();
      drush_include_engine('drupal', 'cron_key');
      drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
    }
//    drush_set_option('installed', TRUE, 'site');

    _provision_client_create_symlink();
  }
}
