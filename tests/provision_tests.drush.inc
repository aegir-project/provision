<?php
/**
 * @file
 *  Some tests for hostmaster and provison.
 *
 *  These could live in Hostmaster or Provision, and there are advantages and
 *  disadvantages to both. But I decided that I'd just get on with it and pop
 *  them into Provision.
 */

define('PROVISION_TESTS_BUILDS_REPO', 'https://github.com/mig5/builds/raw/master');

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
      // Remove the tests for now, the D7 branch is a long way off being able to run
      // these.
      //'hosting',
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
  // Remove the tests for now, the D7 branch is a long way off being able to run
  // these.
  //// Disable the tasks queue, we run them manually instead.
  //// Todo: turn this back on after the tests have run.
  //provision_backend_invoke('@hostmaster', 'vset', array('hosting_queue_tasks_enabled', '0'));
  //
  //if (version_compare(PHP_VERSION, '5.3.0', '<')) {
  //  drush_provision_tests_install_platform('drupal5');
  //}
  //drush_provision_tests_install_platform('drupal6');
  //drush_provision_tests_install_platform('drupal7');
  //drush_provision_tests_install_platform('openatrium');
  //
  //// Install some sites.
  //if (version_compare(PHP_VERSION, '5.3.0', '<')) {
  //  drush_provision_tests_install_site('drupal5', 'default');
  //}
  //drush_provision_tests_install_site('drupal6', 'default');
  //drush_provision_tests_install_site('drupal7', 'standard');
  //drush_provision_tests_install_site('openatrium', 'openatrium');
  //
  //// Remove the sites.
  //if (version_compare(PHP_VERSION, '5.3.0', '<')) {
  //  drush_provision_tests_remove_site('drupal5', 'default');
  //}
  //drush_provision_tests_remove_site('drupal6', 'default');
  //drush_provision_tests_remove_site('drupal7', 'standard');
  //drush_provision_tests_remove_site('openatrium', 'openatrium');
  //
  //// Clean up a little.
  //if (version_compare(PHP_VERSION, '5.3.0', '<')) {
  //  drush_provision_tests_remove_platform('drupal5');
  //}
  //drush_provision_tests_remove_platform('drupal6');
  //drush_provision_tests_remove_platform('drupal7');
  //drush_provision_tests_remove_platform('openatrium');
  //
  //if (drush_get_error() != DRUSH_SUCCESS) {
  //  return drush_set_error(drush_get_error(), 'Running tests failed');
  //}
}

/**
 * Helper function to install a platform.
 */
function drush_provision_tests_install_platform($platform_name) {
  $args = array(
    PROVISION_TESTS_BUILDS_REPO . "/$platform_name.build",
    "/var/aegir/platforms/$platform_name"
  );
  drush_backend_invoke('make', $args);
  $args = array(
    'root' => "/var/aegir/platforms/$platform_name",
    "@platform_$platform_name",
    'context_type' => 'platform',
  );
  drush_backend_invoke('provision-save', $args);
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@platform_$platform_name",));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to remove a platform.
 */
function drush_provision_tests_remove_platform($platform_name) {
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'delete'), array('force' => TRUE));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to install a site.
 */
function drush_provision_tests_install_site($platform_name, $profile_name) {
  $args = array(
    'uri' => "$platform_name.mig5.net",
    "@$platform_name.mig5.net",
    'context_type' => 'site',
    'platform' => "@platform_$platform_name",
    'profile' => $profile_name,
    'db_server' => '@server_localhost',
    'root' => "/var/aegir/platforms/$platform_name",
  );
  drush_backend_invoke('provision-save', $args);
  provision_backend_invoke("@$platform_name.mig5.net", 'provision-install');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'verify'), array('force' => TRUE));
  drush_provision_tests_run_remaining_tasks();
}

/**
 * Helper function to delete a site.
 */
function drush_provision_tests_remove_site($platform_name, $profile_name) {
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$platform_name.mig5.net", 'delete'), array('force' => TRUE));
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
    provision_backend_invoke('@hostmaster', "hosting-task", array($task->nid), array('force' => TRUE));
  }
}

