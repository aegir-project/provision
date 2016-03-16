<?php
/**
 * @file
 * Some tests for hostmaster and provison.
 *
 * These could live in Hostmaster or Provision, and there are advantages and
 * disadvantages to both. But I decided that I'd just get on with it and pop
 * them into Provision.
 */

define('PROVISION_TESTS_BUILDS_REPO', dirname(__FILE__) . '/makes');
define('PROVISION_TESTS_DEFAULT_LOCK_WAIT', 60);
define('PROVISION_TESTS_LOCK_TIMEOUT', 3600.0);

/**
 * Implements hook_drush_command().
 */
function provision_tests_drush_command() {
  $items['provision-tests-run'] = array(
    'description' => dt('Runs provision tests'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    // Although we're a provision command, we require hostmaster to be around to
    // run the tests correctly.
    'drupal dependencies' => array(
      'hosting',
    ),
    'options' => array(
      'lock-wait' => dt('Time to wait to acquire a lock on the task queue. Defaults to :wait seconds.', array(':wait' => PROVISION_TESTS_DEFAULT_LOCK_WAIT)),
      'force' => dt('Continue even if the task queue is still locked.'),
    ),
  );
  $items['provision-tests-new-run'] = array(
    'description' => dt('Runs NEW provision tests'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    // Although we're a provision command, we require hostmaster to be around to
    // run the tests correctly.
    'drupal dependencies' => array(
      'hosting',
    ),
    'options' => array(
      'lock-wait' => dt('Time to wait to acquire a lock on the task queue. Defaults to :wait seconds.', array(':wait' => PROVISION_TESTS_DEFAULT_LOCK_WAIT)),
      'force' => dt('Continue even if the task queue is still locked.'),
    ),
  );
  $items['provision-demo-content'] = array(
    'description' => dt('Setup some demo content on a fresh Aegir install'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    // Although we're a provision command, we require hostmaster to be around to
    // run the tests correctly.
    'drupal dependencies' => array(
      'hosting',
    ),
    'options' => array(
      'lock-wait' => dt('Time to wait to acquire a lock on the task queue. Defaults to :wait seconds.', array(':wait' => PROVISION_TESTS_DEFAULT_LOCK_WAIT)),
      'force' => dt('Continue even if the task queue is still locked.'),
    ),
  );

  return $items;
}

/**
 * Drush command to run the provision tests.
 */
function drush_provision_tests_run() {
  if (!drush_confirm(dt('This command should only be run on a clean Aegir install, and data may be lost! Do you want to continue?'))) {
    return drush_user_abort();
  }
  // Disable the tasks queue, we run them manually instead.
  $queue_status_initial = variable_get('hosting_queue_tasks_enabled', '0');
  variable_set('hosting_queue_tasks_enabled', '0');
  $lock_wait = drush_get_option('lock-wait', PROVISION_TESTS_DEFAULT_LOCK_WAIT);
  if (!lock_wait('hosting_queue_tasks_running', $lock_wait) || drush_get_option('force', FALSE)) {
    if (lock_acquire('hosting_queue_tasks_running', PROVISION_TESTS_LOCK_TIMEOUT)) {
      drush_log('Acquired lock on task queue.');
    }
    elseif (drush_get_option('force', FALSE)) {
      drush_log('Bypassing lock on task queue.', 'warning');
    }
    else {
      drush_die(dt('Cannot acquire lock on task queue.'));
    }
  }
  else {
    drush_die(dt("Cannot acquire lock on task queue after waiting :wait seconds. A longer wait time can be set with the --lock-wait option.", array(':wait' => $lock_wait)));
  }

  drush_provision_tests_add_custom_test_task();

  drush_provision_tests_install_platform('drupal6');
  drush_provision_tests_install_platform('drupal7');

  // Install some sites.
  drush_provision_tests_install_site('drupal6', 'drupal6-default', 'default');
  drush_provision_tests_install_site('drupal7', 'drupal7-standard', 'standard');
  drush_provision_tests_install_site('drupal7', 'drupal7-minimal', 'minimal');

  // Remove the sites.
  drush_provision_tests_remove_site('drupal6-default');
  drush_provision_tests_remove_site('drupal7-standard');
  drush_provision_tests_remove_site('drupal7-minimal');

  // Create some sites and migrate them.
  drush_provision_tests_install_platform('drupal7', 'drupal7_other');
  drush_provision_tests_install_site('drupal7', 'drupal7-migrate-drupal7-other', 'standard');
  drush_provision_tests_dl_module_to_site('token', 'drupal7-migrate-drupal7-other');
  drush_provision_tests_migrate_site('drupal7-migrate-drupal7-other', 'drupal7_other');
  drush_provision_tests_en_module_on_site('token', 'drupal7-migrate-drupal7-other');
  drush_provision_tests_remove_site('drupal7-migrate-drupal7-other');
  drush_provision_tests_remove_platform('drupal7_other');

  // Create some sites, and upgrade them.
  drush_provision_tests_install_site('drupal6', 'drupal6-upgrade-drupal7', 'default');
  drush_provision_tests_migrate_site('drupal6-upgrade-drupal7', 'drupal7');
  drush_provision_tests_remove_site('drupal6-upgrade-drupal7');

  // Clean up a little.
  drush_provision_tests_remove_platform('drupal6');
  drush_provision_tests_remove_platform('drupal7');

  // Restore the tasks queue status.
  variable_set('hosting_queue_tasks_enabled', $queue_status_initial);
  drush_log(dt('Releasing lock on task queue.'));
  lock_release('hosting_queue_tasks_running');

  if (drush_get_error() != DRUSH_SUCCESS) {
    drush_set_error(drush_get_error(), 'Running tests failed');
    exit(1);
  }

  drush_log(dt('Tests completed successfully'), 'success');
}


/**
 * Drush command to run the provision tests.
 */
function drush_provision_tests_new_run() {
  if (!drush_confirm(dt('This command should only be run on a clean Aegir install, and data may be lost! Do you want to continue?'))) {
    return drush_user_abort();
  }

  // Disable the tasks queue, we run them manually instead.
  $queue_status_initial = variable_get('hosting_queue_tasks_enabled', '0');
  variable_set('hosting_queue_tasks_enabled', '0');
  drush_log('Acquiring tasks lock.');
  $lock_wait = drush_get_option('lock-wait', PROVISION_TESTS_DEFAULT_LOCK_WAIT);
  if (!lock_wait('hosting_queue_tasks_running', $lock_wait) || drush_get_option('force', FALSE)) {
    if (lock_acquire('hosting_queue_tasks_running', PROVISION_TESTS_LOCK_TIMEOUT)) {
      drush_log('Acquired lock on task queue.');
    }
    elseif (drush_get_option('force', FALSE)) {
      drush_log('Bypassing lock on task queue.', 'warning');
    }
    else {
      drush_die(dt('Cannot acquire lock on task queue.'));
    }
  }
  else {
    drush_die(dt("Cannot acquire lock on task queue after waiting :wait seconds. A longer wait time can be set with the --lock-wait option.", array(':wait' => $lock_wait)));
  }

  // 'Stable' D8 e.g. latest (beta) release.
  drush_provision_tests_install_platform('drupal8');
  drush_provision_tests_install_site('drupal8', 'drupal8-minimal', 'standard');

  // Dev D8 latest git commit.
  drush_provision_tests_install_platform('drupal8dev');
  drush_provision_tests_install_site('drupal8dev', 'drupal8-dev-minimal', 'standard');

  // Migrate a D8 site from the current release to the latest commit.
  drush_provision_tests_install_site('drupal8', 'drupal8-upgrade-d8', 'standard');
  drush_provision_tests_migrate_site('drupal8-upgrade-d8', 'drupal8dev');

  // Cleanup.
  drush_provision_tests_remove_site('drupal8-upgrade-d8');
  drush_provision_tests_remove_site('drupal8-minimal');
  drush_provision_tests_remove_site('drupal8-dev-minimal');
  drush_provision_tests_remove_platform('drupal8');
  drush_provision_tests_remove_platform('drupal8dev');

  // Restore the tasks queue status:
  variable_set('hosting_queue_tasks_enabled', $queue_status_initial);
  drush_log('Releasing tasks lock.');
  lock_release('hosting_queue_tasks_running');

  if (drush_get_error() != DRUSH_SUCCESS) {
    drush_set_error(drush_get_error(), 'Running tests failed');
    exit(1);
  }

  drush_log(dt('Tests completed successfully'), 'success');
}


/**
 * Drush command to setup some demo platforms and sites.
 *
 * To use these demo sites let names like drupal7-standard.aegir.example.com
 * resolve to the IP of this Aegir server. E.g. in your hosts file.
 */
function drush_provision_tests_provision_demo_content() {
  if (!drush_confirm(dt('This command should only be run on a clean Aegir install, and data may be lost! Do you want to continue?'))) {
    return drush_user_abort();
  }

  // Disable the tasks queue, we run them manually instead.
  $queue_status_initial = variable_get('hosting_queue_tasks_enabled', '0');
  variable_set('hosting_queue_tasks_enabled', '0');
  drush_log('Acquiring tasks lock.');
  $lock_wait = drush_get_option('lock-wait', PROVISION_TESTS_DEFAULT_LOCK_WAIT);
  if (!lock_wait('hosting_queue_tasks_running', $lock_wait) || drush_get_option('force', FALSE)) {
    if (lock_acquire('hosting_queue)tasks_running', PROVISION_TESTS_LOCK_TIMEOUT)) {
      drush_log('Acquired lock on task queue.');
    }
    elseif (drush_get_option('force', FALSE)) {
      drush_log('Bypassing lock on task queue.', 'warning');
    }
    else {
      drush_die(dt('Cannot acquire lock on task queue.'));
    }
  }
  else {
    drush_die(dt("Cannot acquire lock on task queue after waiting :wait seconds. A longer wait time can be set with the --lock-wait option.", array(':wait' => $lock_wait)));
  }

  // Start with D8 as it needs the most work/testing.
  drush_provision_tests_install_platform('drupal8');
  drush_provision_tests_install_site('drupal8', 'drupal8-minimal', 'minimal');

  // Prepare some platforms.
  drush_provision_tests_install_platform('drupal6');
  drush_provision_tests_install_platform('drupal7');
  drush_provision_tests_install_platform('openatrium2');

  // Install some sites.
  drush_provision_tests_install_site('drupal6', 'drupal6-default', 'default');
  drush_provision_tests_install_site('drupal7', 'drupal7-standard', 'standard');
  drush_provision_tests_install_site('drupal7', 'drupal7-minimal', 'minimal');
  drush_provision_tests_install_site('openatrium2', 'openatrium2-openatrium', 'openatrium');

  // Restore the tasks queue status:
  variable_set('hosting_queue_tasks_enabled', $queue_status_initial);
  drush_log('Releasing tasks lock.');
  lock_release('hosting_queue_tasks_running');

  if (drush_get_error() != DRUSH_SUCCESS) {
    drush_set_error(drush_get_error(), 'Setting up demo content failed');
    exit(1);
  }

  drush_log(dt('Demo content setup successfully'), 'success');
}

/**
 * Helper function to install a platform.
 */
function drush_provision_tests_install_platform($platform_name, $platform_alias = NULL) {
  if (is_null($platform_alias)) {
    $platform_alias = $platform_name;
  }
  drush_log(dt('Building platform: @platform and adding to hostmaster.', array('@platform' => $platform_alias)), 'ok');
  $args = array(
    PROVISION_TESTS_BUILDS_REPO . "/$platform_name.make",
    "/var/aegir/platforms/$platform_alias",
  );
  $opts = array();
  if (version_compare(drush_core_version(), '7', '>=')) {
    $opts[] = '--shallow-clone';
  }
  drush_invoke_process('@none', 'make', $args, $opts);
  $args = array(
    "@platform_$platform_alias",
  );
  $options = array(
    'root' => "/var/aegir/platforms/$platform_alias",
    'context_type' => 'platform',
  );
  drush_invoke_process('@none', 'provision-save', $args, $options);
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@platform_$platform_alias"));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to remove a platform.
 */
function drush_provision_tests_remove_platform($platform_name) {
  drush_log(dt('Removing platform: @platform.', array('@platform' => $platform_name)), 'ok');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'delete'), array('force' => TRUE));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to install a site.
 */
function drush_provision_tests_install_site($platform_name, $site, $profile_name) {
  drush_log(dt('Installing: @site on platform: @platform with profile: @profile.', array('@site' => "$site.aegir.example.com", '@platform' => $platform_name, '@profile' => $profile_name)), 'ok');
  $args = array(
    "@$site.aegir.example.com",
  );
  $options = array(
    'uri' => "$site.aegir.example.com",
    'context_type' => 'site',
    'platform' => "@platform_$platform_name",
    'profile' => $profile_name,
    'db_server' => '@server_localhost',
    'root' => "/var/aegir/platforms/$platform_name",
    'client_email' => 'this.email@is.invalid',
  );
  drush_invoke_process('@none', 'provision-save', $args, $options);

  $args = array();
  $options = array(
    'client_email' => 'this.email@is.invalid',
  );
  provision_backend_invoke("@$site.aegir.example.com", 'provision-install', $args, $options);
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'verify'), array('force' => TRUE));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to download a module to a site.
 */
function drush_provision_tests_dl_module_to_site($module, $site) {
  drush_log(dt('Downloading module: @module to site: @site.', array('@site' => "$site.aegir.example.com", '@module' => $module)), 'ok');
  $args = array(
    $module,
  );
  $options = array(
    // Normally 'dl' in a site context should know to put it in the site-
    // specific modules directory, but not through provision_backend_invoke()?
    'destination' => "sites/$site.aegir.example.com/modules",
  );
  provision_backend_invoke("@$site.aegir.example.com", 'dl', $args, $options);
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to enable a module on a site.
 */
function drush_provision_tests_en_module_on_site($module, $site) {
  drush_log(dt('Enabling module: @module on site: @site.', array('@site' => "$site.aegir.example.com", '@module' => $module)), 'ok');
  $args = array(
    $module,
  );
  provision_backend_invoke("@$site.aegir.example.com", 'en', $args);
  // Failing 'en' only results in a warning, so we need to set our own error.
  $log = array_slice(drush_get_log(), 10);
  foreach ($log as $serial => $entry) {
    if ($entry['type'] == 'warning' && $entry['message'] == "$module was not found and will not be enabled.") {
      drush_set_error('DRUSH_FRAMEWORK_ERROR', "Error enabling module: $module on site: $site.");
    }
  }
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to delete a site.
 */
function drush_provision_tests_remove_site($site) {
  drush_log(dt('Removing: @site.', array('@site' => "$site.aegir.example.com")), 'ok');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$site.aegir.example.com", 'delete'), array('force' => TRUE));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Migrates a site from one platform to another.
 *
 * @param string $site
 *   The site to migrate.
 * @param string $target
 *   The target platform to migrate to.
 */
function drush_provision_tests_migrate_site($site, $target) {
  drush_log(dt('Migrating: @site to platform: @platform.', array('@site' => "$site.aegir.example.com", '@platform' => $target)), 'ok');
  // Do the migrate.
  provision_backend_invoke("@$site.aegir.example.com", 'provision-migrate', array("@platform_$target"));
  // Import the site into the frontend.
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@$site.aegir.example.com"));
  // Verify the $target platform.
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$target", 'verify'), array('force' => TRUE));
  // Import and verify the site.
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@$site.aegir.example.com"));
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$site.aegir.example.com", 'verify'), array('force' => TRUE));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Create a custom verify task for the hostmaster site and place it in the queue.
 */
function drush_provision_tests_add_custom_test_task() {
  drush_log(dt("Create a custom verify task for the hostmaster site and place it in the queue."), 'ok');

  $node = new stdClass();

  // Admin
  $node->uid = 1;

  $node->type = 'task';

  // The site, platform or server node ID that is subject to the task.
  // 10 usually is the node ID for the hostmaster site itself.
  $node->rid = 10;

  // Published status == 1
  $node->status = 1;

  $node->task_type = 'verify';

  // Setting status to HOSTING_TASK_QUEUED == 0
  $node->task_status = 0;

  node_save($node);

  drush_provision_tests_run_remaining_tasks(TRUE);

  drush_log(dt("Finished running a custom verify task for the hostmaster site."), 'ok');
}

/**
 * Run all remaining hosting tasks.
 */
function drush_provision_tests_run_remaining_tasks($debug = FALSE) {
  provision_backend_invoke('@hostmaster', "hosting-tasks", array(), array('force' => TRUE, 'debug' => $debug));
}
