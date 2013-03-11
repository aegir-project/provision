<?php

/**
 * Prompt the user to confirm.
 *
 * Here we ask to confirm, and also ask if all sites and platforms
 * also need to be trashed.
 */
function drush_provision_hostmaster_uninstall_validate($site = NULL) {
  drush_print(dt('This command will DELETE YOUR MAIN AEGIR SITE!

This command will:

 1. delete the Aegir site (database, database user and files)
 2. remove the dispatcher crontab'));

  if (drush_get_option('all')) {
    drush_print(dt(' 3. DELETE ALL PLATFORMS AND SITES MANAGED BY AEGIR!
'));
  } else {
    drush_print(dt('

The hostmaster platform and other platforms and sites managed by Aegir will be
left for your perusal.
'));
  }

  if (!drush_confirm(dt('Do you want to uninstall Aegir?'))) {
    return drush_set_error('PROVISION_CANCEL_UNINSTALL', dt('Uninstall aborted'));
  }
  if (drush_get_option('all') && !drush_confirm(dt('REALLY? Do you REALLY want to destroy all sites managed by Aegir?'))) {
    return drush_set_error('PROVISION_CANCEL_UNINSTALL', dt('Uninstall aborted'));
  }
  return TRUE;
}

function drush_provision_hostmaster_uninstall() {
  if (drush_get_option('all')) {
    // need to access the hostmaster database
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_DATABASE);

    drush_log(dt('Destroying sites'), 'info');
    $query = db_query("SELECT n.title AS uri, h.name AS context FROM {node} n INNER JOIN {hosting_site} s ON s.nid=n.nid LEFT JOIN {hosting_context} h ON h.nid = n.nid WHERE n.type = '%s' AND s.status != %d AND h.name != 'hostmaster'", 'site', -2);
    while ($resource = $query->fetch()) {
      drush_log(dt("Destroying @resource", array('@resource' => $resource->uri)));
      $context = $resource->context;
      if (!$context) {
        drush_log(dt('Context missing for resource @resource, guessing', array('@resource' => $resource->uri)), 'warning');
        $context = $resource->uri;
      }
      provision_backend_invoke($context, 'provision-delete');
    }

    drush_log(dt('Destroying platforms'), 'info');
    $query = db_query("SELECT n.title AS uri, h.name AS context FROM {node} n INNER JOIN {hosting_platform} p LEFT JOIN {hosting_context} h ON h.nid = n.nid WHERE n.type = '%s' AND p.status != %d AND h.name != '%s'", 'platform', -2, d('@hostmaster')->platform->name);
    while ($resource = $query->fetch()) {
      drush_log(dt("Destroying @resource", array('@resource' => $resource->uri)));
      $context = $resource->context;
      if (!$context) {
        drush_log(dt('Context missing for platform @resource, ignoring', array('@resource' => $resource->uri)), 'warning');
      } else {
        provision_backend_invoke($context, 'provision-delete');
      }
    }

    drush_log(dt('Destroying servers'), 'info');
    $query = db_query("SELECT n.title AS uri, h.name AS context FROM {node} n LEFT JOIN {hosting_context} h ON h.nid = n.nid WHERE n.type = '%s' AND h.name != '%s' AND h.name != '%s'", 'server', d('@hostmaster')->server->name, d('@hostmaster')->db_server->name);
    while ($resource = $query->fetch()) {
      drush_log(dt("Destroying @resource", array('@resource' => $resource->uri)));
      $context = $resource->context;
      if (!$context) {
        drush_log(dt('Context missing for platform @resource, ignoring', array('@resource' => $resource->uri)), 'warning');
      } else {
        provision_backend_invoke($context, 'provision-delete');
      }
    }
  }

  drush_log(dt('Destroying main hostmaster site'), 'info');
  provision_backend_invoke('@hostmaster', 'provision-delete');
  drush_log(dt('Removing crontab'), 'info');
  exec('crontab -r');
  drush_log(dt('Removing alias'), 'info');
  $config = new Provision_Config_Drushrc_Alias('@hostmaster');
  $config->unlink();
}
