<?php
// $Id$

require_once(dirname(__FILE__) . '/../provision.inc');
if ($argv[1]) {
  // Fake the necessary HTTP headers that Drupal needs:
  provision_external_init($argv[1], FALSE);

}
else {
  provision_set_error(PROVISION_FRAMEWORK_ERROR);
  provision_log("error", "USAGE: update.php url\n");
  provision_output($url, $data);
}

/**
 * @file
 *   Update.php for provisioned sites.
 *   This file is a derivative of the standard drupal update.php,
 *   which has been modified to allow being run from the command
 *   line.
 */

ob_start();
$_REQUEST['op'] = 'info';
include_once("update.php");
ob_end_clean();

function update_main() {
  include_once './includes/install.inc';
  include_once './includes/batch.inc';
  drupal_load_updates();

  update_fix_d6_requirements();
  update_fix_compatibility();

  $start = array();
  $modules = drupal_get_installed_schema_version(NULL, FALSE, TRUE);
  foreach ($modules as $module => $schema_version) {
    $updates = drupal_get_schema_versions($module);
    // Skip incompatible module updates completely, otherwise test schema versions.
    if (!update_check_incompatibility($module) && $updates !== FALSE && $schema_version >= 0) {
      // module_invoke returns NULL for nonexisting hooks, so if no updates
      // are removed, it will == 0.
      $last_removed = module_invoke($module, 'update_last_removed');
      if ($schema_version < $last_removed) {
        provision_set_error(PROVISION_INSTALL_ERROR);
        provision_log('error', pt( $module .' module can not be updated. Its schema version is '. $schema_version .'. Updates up to and including '. $last_removed .' have been removed in this release. In order to update '. $module .' module, you will first <a href="http://drupal.org/upgrade">need to upgrade</a> to the last version in which these updates were available.'));
        continue;
      }

      $updates = drupal_map_assoc($updates);
      $updates[] = 'No updates available';
      $default = $schema_version;
      foreach (array_keys($updates) as $update) {
        if ($update > $schema_version) {
          $default = $update;
          break;
        }
      }
      $start[$module] = $default;
    }
  }
  if (sizeof($start)) {
    $operations = array();
    foreach ($start as $module => $version) {
      drupal_set_installed_schema_version($module, $version - 1);
      $updates = drupal_get_schema_versions($module);
      $max_version = max($updates);
      if ($version <= $max_version) {
        provision_log('notice', pt('Updating module @module from schema version @start to schema version @max', array('@module' => $module, '@start' => $version - 1, '@max' => $max_version)));
        foreach ($updates as $update) {
          if ($update >= $version) {
            $operations[] = array('_update_do_one', array($module, $update));
          }
        }
      }
      else {
        provision_log('notice', pt('No updates for module @module', array('@module' => $module)));
      }
    }
    $batch = array(
      'operations' => $operations,
      'title' => 'Updating',
      'init_message' => 'Starting updates',
      'error_message' => 'An unrecoverable error has occurred. You can find the error message below. It is advised to copy it to the clipboard for reference.',
      'finished' => 'update_finished',
    );
    batch_set($batch);
    $batch =& batch_get();
    $batch['progressive'] = FALSE;
    batch_process();
  }
  else {
    provision_log('notice', pt('No outstanding updates'));
  }
}

/**
 * A simplified version of the batch_do_one function from update.php
 * 
 * This does not mess with sessions and the like, as it will be used
 * from the command line
 */
function _update_do_one($module, $number, &$context) {
  // If updates for this module have been aborted
  // in a previous step, go no further.
  if (!empty($context['results'][$module]['#abort'])) {
    return;
  }

  $function = $module .'_update_'. $number;
  if (function_exists($function)) {
    $ret = $function($context['sandbox']);
  }
  foreach ($ret as $info) {
    if (!$info['success']) {
      provision_set_error('PROVISION_DB_ERROR');
    }
    provision_log( ($info['success']) ? 'success' : 'error', $info['query']);
  }

  if (isset($ret['#finished'])) {
    $context['finished'] = $ret['#finished'];
    unset($ret['#finished']);
  }

  if ($context['finished'] == 1 && empty($context['results'][$module]['#abort'])) {
    drupal_set_installed_schema_version($module, $number);
  }

}

update_main($url, $data);
provision_output($url, $data);
