<?php
/**
 * @file
 *  Some tests for hostmaster and provison.
 *
 *  These could live in Hostmaster or Provision, and there are advantages and
 *  disadvantages to both. But I decided that I'd just get on with it and pop
 *  them into Provision.
 */

define('PROVISION_TESTS_BUILDS_REPO', dirname(__FILE__) . 'provision-tests/makes');

/**
 * Implementation of hook_drush_command().
 */
function provision_tests_drush_command() {
  $items['provision-tests-run'] = array(
    'description' => dt('Runs provision tests'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    // Although we're a provision command, we require hostmaster to be around to
    // run the tests correctly
    'drupal dependencies' => array(
      'hosting',
    ),
  );

  return $items;
}

/**
 * Drush command to run the provision tests.
 */
function drush_provision_tests_run() {
  if (version_compare(PHP_VERSION, '5.3.0', '<')) {
    $drupal5_tests = TRUE;
  }
  else {
    $drupal5_tests = FALSE;
    drush_log(dt('Please note that because you are running PHP 5.3 or greater, you cannot test installing Drupal 5 on this machine.'), 'warning');
  }

  if (!drush_confirm(dt('This command should only be run on a clean Aegir install, and data may be lost! Do you want to continue?'))) {
    return drush_user_abort();
  }

  // Disable the tasks queue, we run them manually instead.
  $queue_status_initial = variable_get('hosting_queue_tasks_enabled', '0');
  variable_set('hosting_queue_tasks_enabled', '0');

  if ($drupal5_tests) {
    drush_provision_tests_install_platform('drupal5');
  }
  drush_provision_tests_install_platform('drupal6');
  drush_provision_tests_install_platform('drupal7');
  drush_provision_tests_install_platform('openatrium');

  // Install some sites.
  if ($drupal5_tests) {
    drush_provision_tests_install_site('drupal5', 'drupal5-default', 'default');
  }
  drush_provision_tests_install_site('drupal6', 'drupal6-default', 'default');
  drush_provision_tests_install_site('drupal7', 'drupal7-standard', 'standard');
  drush_provision_tests_install_site('drupal7', 'drupal7-minimal', 'minimal');
  drush_provision_tests_install_site('openatrium', 'openatrium-openatrium', 'openatrium');

  // Remove the sites.
  if ($drupal5_tests) {
    drush_provision_tests_remove_site('drupal5-default', 'default');
  }
  drush_provision_tests_remove_site('drupal6-default');
  drush_provision_tests_remove_site('drupal7-standard');
  drush_provision_tests_remove_site('drupal7-minimal');
  drush_provision_tests_remove_site('openatrium-openatrium');

  // Create some sites and migrate them.
  if ($drupal5_tests) {
    drush_provision_tests_install_platform('drupal5', 'drupal5_other');
    drush_provision_tests_install_site('drupal5', 'drupal5-migrate-drupal5-other', 'default');
    drush_provision_tests_migrate_site('drupal5-migrate-drupal5-other', 'drupal5_other');
    drush_provision_tests_remove_site('drupal5-migrate-drupal5-other');
    drush_provision_tests_remove_platform('drupal5_other');
  }
  drush_provision_tests_install_platform('drupal6', 'drupal6_other');
  drush_provision_tests_install_site('drupal6', 'drupal6-migrate-drupal6-other', 'default');
  drush_provision_tests_migrate_site('drupal6-migrate-drupal6-other', 'drupal6_other');
  drush_provision_tests_remove_site('drupal6-migrate-drupal6-other');
  drush_provision_tests_remove_platform('drupal6_other');
  drush_provision_tests_install_platform('drupal7', 'drupal7_other');
  drush_provision_tests_install_site('drupal7', 'drupal7-migrate-drupal7-other', 'standard');
  drush_provision_tests_migrate_site('drupal7-migrate-drupal7-other', 'drupal7_other');
  drush_provision_tests_remove_site('drupal7-migrate-drupal7-other');
  drush_provision_tests_remove_platform('drupal7_other');


  // Create some sites, and upgrade them
  if ($drupal5_tests) {
    drush_provision_tests_install_site('drupal5', 'drupal5-upgrade-drupal7', 'default');
    drush_provision_tests_migrate_site('drupal5-upgrade-drupal7', 'drupal6');
    drush_provision_tests_migrate_site('drupal5-upgrade-drupal7', 'drupal7');
    drush_provision_tests_remove_site('drupal5-upgrade-drupal7');
  }
  drush_provision_tests_install_site('drupal6', 'drupal6-upgrade-drupal7', 'default');
  drush_provision_tests_migrate_site('drupal6-upgrade-drupal7', 'drupal7');
  drush_provision_tests_remove_site('drupal6-upgrade-drupal7');

  // Clean up a little.
  if ($drupal5_tests) {
    drush_provision_tests_remove_platform('drupal5');
  }
  drush_provision_tests_remove_platform('drupal6');
  drush_provision_tests_remove_platform('drupal7');
  drush_provision_tests_remove_platform('openatrium');

  // Restore the tasks queue status:
  variable_set('hosting_queue_tasks_enabled', $queue_status_initial);

  if (drush_get_error() != DRUSH_SUCCESS) {
    return drush_set_error(drush_get_error(), 'Running tests failed');
  }

  drush_log(dt('Tests completed successfully'), 'success');
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
    PROVISION_TESTS_BUILDS_REPO . "/$platform_name.build",
    "/var/aegir/platforms/$platform_alias"
  );
  drush_backend_invoke('make', $args);
  $args = array(
    'root' => "/var/aegir/platforms/$platform_alias",
    "@platform_$platform_alias",
    'context_type' => 'platform',
  );
  drush_backend_invoke('provision-save', $args);
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@platform_$platform_alias",));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to remove a platform.
 */
function drush_provision_tests_remove_platform($platform_name) {
  drush_log(dt('Removing platform: @platform.', array('@platform' => $platform_name)), 'ok');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'delete'));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to install a site.
 */
function drush_provision_tests_install_site($platform_name, $site, $profile_name) {
  drush_log(dt('Installing: @site on platform: @platform with profile: @profile.', array('@site' => "$site.aegir.example.com", '@platform' => $platform_name, '@profile' => $profile_name)), 'ok');
  $args = array(
    'uri' => "$site.aegir.example.com",
    "@$site.aegir.example.com",
    'context_type' => 'site',
    'platform' => "@platform_$platform_name",
    'profile' => $profile_name,
    'db_server' => '@server_localhost',
    'root' => "/var/aegir/platforms/$platform_name",
  );
  drush_backend_invoke('provision-save', $args);
  provision_backend_invoke("@$site.aegir.example.com", 'provision-install');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'verify'));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to delete a site.
 */
function drush_provision_tests_remove_site($site) {
  drush_log(dt('Removing: @site.', array('@site' => "$site.aegir.example.com")), 'ok');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$site.aegir.example.com", 'delete'));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Migrates a site from one platform to another.
 *
 * @param $site
 *   The site to migrate.
 * @param $target
 *   The target platform to migrate to.
 */
function drush_provision_tests_migrate_site($site, $target) {
  drush_log(dt('Migrating: @site to platform: @platform.', array('@site' => "$site.aegir.example.com", '@platform' => $target)), 'ok');
  // Do the migrate.
  provision_backend_invoke("@$site.aegir.example.com", 'provision-migrate', array("@platform_$target",));
  // Import the site into the frontend.
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@$site.aegir.example.com",));
  // Verify the $target platform.
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$target", 'verify'));
  // Import and verify the site.
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@$site.aegir.example.com",));
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$site.aegir.example.com", 'verify'));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Run all remaining hosting tasks.
 */
function drush_provision_tests_run_remaining_tasks() {
  $tasks = array();
  $result = db_query("SELECT t.nid FROM {hosting_task} t INNER JOIN {node} n ON t.vid = n.vid WHERE t.task_status = %d ORDER BY n.changed, n.nid ASC", 0);
  while ($node = db_fetch_object($result)) {
    $tasks[$node->nid] =  node_load($node->nid);
  }

  foreach ($tasks as $task) {
    provision_backend_invoke('@hostmaster', "hosting-task", array($task->nid));
  }
}
