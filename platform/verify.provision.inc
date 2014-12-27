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
    provision_drupal_push_site();

    drush_errors_on();
    $rrFile = d()->root . '/sites/all/modules/registry-rebuild.ini';
    if (provision_file()->exists($rrFile)->status()) {
      if (drush_drupal_major_version() < 8 && d()->profile != 'hostmaster') {
        provision_backend_invoke(d()->name, 'registry-rebuild');
        drush_log(dt('Completed registry-rebuild without --no-cache-clear option'));
      }
    }
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
    provision_file()->create_dir(d()->clients_path, dt('Provision client home'), 0750);
    if (d()->name == '@server_master') {
      provision_file()->create_dir(d()->backup_path, dt('Backup'), 0750);
      provision_file()->create_dir(d()->include_path, dt('Provision PHP configuration'), 0711);
      if (!provision_file()->exists(d()->include_path . '/global.inc')->succeed('Global configuration file exists')->status()) {
        // Create an empty global.inc so the include doesn't fail with
        // open_basedir restrictions
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
    // create a platform from a makefile, if it doesnt already exist and the makefile does.
    if (!provision_file()->exists(d()->root)->status() && !empty(d()->makefile)) {
      drush_log(dt("Platform path does not exist, fetching from drush makefile"));
      $arguments = array();
      $options = array();
      // propagate working-copy args downward
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
    if(drush_get_error()) {
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

    // Determine latest BOA contrib directory name and add INI template if missing.
    $pathAll = '/data/all/';
    $pathDrc = d()->root . '/sites/all/drush';
    $iniFile = d()->root . '/sites/all/modules/default.boa_platform_control.ini';
    $iniTpl = '/data/conf/default.boa_platform_control.ini';

    if (!provision_file()->exists($iniFile)->status() && provision_file()->exists($iniTpl)->status()) {
      provision_file()->copy($iniTpl, $iniFile);
    }

    if (!preg_match("/\/static\//", d()->root)) {
      if (d()->profile != 'hostmaster') {
        provision_file()->chmod(d()->root . '/sites/all/modules', 02775)
          ->succeed('Changed permissions of <code>@path</code> to @perm')
          ->fail('Could not change permissions of <code>@path</code> to @perm');
        provision_file()->chmod(d()->root . '/sites/all/themes', 02775)
          ->succeed('Changed permissions of <code>@path</code> to @perm')
          ->fail('Could not change permissions of <code>@path</code> to @perm');
        provision_file()->chmod(d()->root . '/sites/all/libraries', 02775)
          ->succeed('Changed permissions of <code>@path</code> to @perm')
          ->fail('Could not change permissions of <code>@path</code> to @perm');
      }
      provision_file()->chmod(d()->root . '/sites/all', 0755)
        ->succeed('Changed permissions of <code>@path</code> to @perm')
        ->fail('Could not change permissions of <code>@path</code> to @perm');
      provision_file()->chmod(d()->root . '/sites', 0751)
        ->succeed('Changed permissions of <code>@path</code> to @perm')
        ->fail('Could not change permissions of <code>@path</code> to @perm');
    }

    if (provision_file()->exists($pathDrc)->status()) {
      provision_file()->chmod(d()->root . '/sites/all/drush', 0700)
        ->succeed('Changed permissions of <code>@path</code> to @perm')
        ->fail('Could not change permissions of <code>@path</code> to @perm');
    }

    if (provision_file()->exists($pathAll)->status() && preg_match("/\/static\//", d()->root)) {
      $sortedDirs = array();
      foreach(scandir($pathAll) as $dir) {
        if(is_dir($pathAll . $dir)) {
          array_push($sortedDirs, $dir);
        }
      }
      $latestDir = max($sortedDirs);
      drush_log(dt('Contrib latestDir: !latest', array('!latest' => $latestDir)), 'message');

      if (is_numeric($latestDir)) {
        $core6_contrib_dir = '/data/all/' . $latestDir . '/o_contrib';
        $core7_contrib_dir = '/data/all/' . $latestDir . '/o_contrib_seven';
      }
      else {
        $core6_contrib_dir = '/data/all/000/modules/o_contrib';
        $core7_contrib_dir = '/data/all/000/modules/o_contrib_seven';
      }

      $core6_version_id = d()->root . '/modules/path_alias_cache';
      $core6_contrib_symlink = d()->root . '/modules/o_contrib';
      $core7_version_id = d()->root . '/modules/overlay';
      $core7_contrib_symlink = d()->root . '/modules/o_contrib_seven';

      // Make sure the core modules directory is writable, but only if it is a BOA system.
      if (provision_file()->exists($core6_contrib_dir)->status() || provision_file()->exists($core7_contrib_dir)->status()) {
        provision_file()->writable(d()->root . '/modules')
          ->succeed('Drupal core modules directory @path is writable by the provisioning script')
          ->fail('Drupal core modules directory @path is not writable by the provisioning script', 'PROVISION_MODULES_DIR_NOT_WRITABLE');
      }

      // Add symlink to BOA specific contrib in Pressflow 6.
      if (provision_file()->exists($core6_contrib_dir)->status() && provision_file()->exists($core6_version_id)->status() && !provision_file()->exists($core6_contrib_symlink)->status()) {
        provision_file()->symlink($core6_contrib_dir, $core6_contrib_symlink)
          ->succeed('Created symlink @path to @target')
          ->fail('Could not create symlink @path to @target: @reason');
      }

      // Add symlink to BOA specific contrib in Drupal 7.
      if (provision_file()->exists($core7_contrib_dir)->status() && provision_file()->exists($core7_version_id)->status() && !provision_file()->exists($core7_contrib_symlink)->status()) {
        provision_file()->symlink($core7_contrib_dir, $core7_contrib_symlink)
          ->succeed('Created symlink @path to @target')
          ->fail('Could not create symlink @path to @target: @reason');
      }
    }
    provision_drupal_push_site();
  }
  elseif (d()->type === 'site') {

    // Re-set the ROOT PATH for Drush.
    // In cases where we just build the platform Drush would have cleared this value.
    // See Drush commit 3b74d40b1228f022464b92243c16127eb613c2df
    drush_set_context('DRUSH_SELECTED_DRUPAL_ROOT', d()->root);

    drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
    // This is the actual drupal provisioning requirements.
    _provision_drupal_create_directories();
    _provision_drupal_maintain_aliases();
    _provision_drupal_ensure_htaccess_update();
    // Requires at least the database settings to complete.

    _provision_drupal_create_settings_file();

    if (function_exists('hosting_get_features')) {
      // We're verifying the hostmaster site, so record a list of enabled
      // Hosting Features.
      $aegir_drushrc = new Provision_Config_Drushrc_Aegir();
      $aegir_drushrc->write();
    }

    provision_drupal_push_site();
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

    $iniFile = d()->root . '/sites/' . d()->uri . '/modules/default.boa_site_control.ini';
    $iniTpl = '/data/conf/default.boa_site_control.ini';

    if (!provision_file()->exists($iniFile)->status() && provision_file()->exists($iniTpl)->status()) {
      provision_file()->copy($iniTpl, $iniFile);
    }
  }
}

