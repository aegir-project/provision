<?php

/**
 * @file
 * Code related to verify tasks.
 *
 * Composer-based Platforms Options
 * =================================
 *
 * There are a few drush options that control how `composer` is run on platform
 * verify:
 *
 *  provision_composer_install_platforms
 *      Set to FALSE to prevent provision from ever running `composer install`.
 *
 *  provision_composer_install_platforms_verify_always
 *      By default, provision will run `composer install` every time a platform
 *      is verified.
 *
 *      Set to FALSE to only run `composer install` once. If composer.json
 *      changes, you will have to run `composer install` manually.
 *
 *  provision_composer_install_command
 *      The specific command to run during platform verify.
 *      Defaults to 'composer install --no-interaction --no-progress --no-dev'
 *
 */

/**
 * Provision verify command.
 *
 * Disable an enabled and installed site
 */
function drush_provision_drupal_provision_verify_validate() {
  if (d()->type === 'site') {
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
    provision_prepare_environment();

    _provision_drupal_create_settings_file();
    provision_drupal_push_site();

    drush_errors_on();
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
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
  
    drush_log(dt("Checking for platform at path @path...", array('@path' => d()->root)), 'debug');
    drush_log(dt("Platform makefile: @makefile", array('@makefile' => d()->makefile)), 'debug');
    
    // Create a platform from a makefile, if it doesnt already exist and the makefile does.
    if (!provision_file()->exists(d()->root)->status() && !empty(d()->makefile)) {
      drush_log(dt("Platform path does not exist, fetching from drush makefile"), 'notice');
      $arguments = array();
      $options = array();
      // Propagate working-copy args downward.
      if (drush_get_option('working-copy') || !empty(d()->make_working_copy)) {
        $options[] = '--working-copy';
      }
      $arguments[] = d()->makefile;
      $arguments[] = d()->root;
      
      // Change current directory to makefile's directory until Drush fixes this bug: https://github.com/drush-ops/drush/issues/2482
      // @TODO: Remove this once is committed.
      if (!empty(d()->makefile) && file_exists(dirname(d()->makefile))) {
        chdir(dirname(d()->makefile));
      }

      drush_invoke_process('@none', "make", $arguments, $options);
      if (drush_get_error()) {
        return drush_set_error("DRUSH_MAKE_FAILED",
          "Could not download platform using drush make. No platform present");
      }
    }

    // Composer Install Support
    // If drush variable provision_composer_install_platforms is not set to FALSE, run composer install.
    if (drush_get_option('provision_composer_install_platforms', TRUE) && shell_exec('which composer')) {
      // Detect composer-based platform and run composer install if it has not been run yet.
      // Step 1: Look for composer directory. Could be Drupal root. Could be the git repo_root.
      if (provision_file()->exists(d()->root . DIRECTORY_SEPARATOR . 'composer.json')->status()) {
        $composer_directory = d()->root;
      }
      elseif (d()->repo_path && provision_file()->exists(d()->repo_path . DIRECTORY_SEPARATOR . 'composer.json')->status()) {
        $composer_directory = d()->repo_path;
      }

      // Step 2: Run composer install if composer.json is present.
      // If drush option provision_composer_install_platforms_verify_always is set to
      // false, only run `composer-install` if ./vendor directory is missing.
      if (isset($composer_directory) && (!file_exists($composer_directory . DIRECTORY_SEPARATOR . 'vendor') || drush_get_option('provision_composer_install_platforms_verify_always', TRUE))) {

        // Composer Install command: Do not interact, do not show download progress.
        // Customizable by setting drush option 'provision_composer_install_command'
        $composer_command = drush_get_option('provision_composer_install_command', 'composer install --no-interaction --no-progress --no-dev');
        drush_log(dt("Running command @command", array(
          '@command' => $composer_command
        )), 'ok');
        $start = time();

        // @TODO: Implement Symfony Process component for line-by-line output logging.
        if (drush_shell_cd_and_exec($composer_directory, $composer_command)) {
          $stop = time();

          $output = implode("\n", drush_shell_exec_output());
          $log_status = strpos($output, 'Warning:') === FALSE? 'success': 'warning';

          drush_log($output, $log_status);
          drush_log(dt(strpos($output, 'Warning:') . "Command ran successfully in @times: @command", array(
            '@command' => $composer_command,
            '@time' => $stop - $start,
          )), $log_status);
        }
        else {
          drush_log(implode("\n", drush_shell_exec_output()), 'error');
          drush_set_error('DRUSH_COMPOSER_ERROR', dt('The composer command failed in @dir: @command', array(
            '@dir' => $composer_directory,
            '@command' => $composer_command,
          )));
        }
      }
    }

    // Re-set the ROOT PATH for Drush.
    // In cases where we just build the platform Drush would have cleared this value.
    // See Drush commit 3b74d40b1228f022464b92243c16127eb613c2df
    drush_set_context('DRUSH_SELECTED_DRUPAL_ROOT', d()->root);

    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_ROOT);
    // If we couldn't bootstrap, there's no sense in continuing past this point.
    if (drush_get_error()) {
      $errors = drush_get_error_log();
      foreach ($errors as $type => &$error) {
        $error = "$type: {$error[0]}";
      }
      return drush_set_error("DRUSH_INVALID_DRUPAL_ROOT",
        dt("Drush could not bootstrap the platform at @path. Please check the platform directory exists and is readable. The errors were: @errors", array(
          '@path' => d()->root,
          '@errors' => implode('|', $errors)
        )));
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
    drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
    // This is the actual drupal provisioning requirements.
    _provision_drupal_create_directories();
    _provision_drupal_maintain_aliases();
    _provision_drupal_ensure_htaccess_update();
    // Requires at least the database settings to complete.

    _provision_drupal_create_settings_file();

    // If this is the hostmaster site, save the ~/.drush/drushrc.php file.
    if (d()->root == d('@hostmaster')->root && d()->uri == d('@hostmaster')->uri) {
      $aegir_drushrc = new Provision_Config_Drushrc_Aegir();
      $aegir_drushrc->write();
    }

    provision_drupal_push_site(drush_get_option('override_slave_authority', FALSE));
  }
}

/**
 * Implements drush_hook_provision_post_COMMAND().
 */
function drush_provision_drupal_post_provision_verify() {
  if (d()->type === 'site') {
    _provision_drupal_rebuild_caches();
    drush_set_option('installed', TRUE, 'site');

    _provision_client_create_symlink();
    drush_include_engine('drupal', 'cron_key');
  }
}
