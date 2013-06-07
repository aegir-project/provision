<?php

/**
 * Implements hook_drush_init().
 */
function subdirs_drush_init() {
  subdirs_provision_register_autoload();
}

/**
 * Register our directory as a place to find Provision classes.
 *
 * This allows Provision to autoload our classes, so that we don't need to
 * specifically include the files before we use the class.
 */
function subdirs_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}


/**
 * Implements drush_hook_pre_provision_install().
 */
function drush_subdirs_pre_provision_install() {
  if (d()->type == 'site') {
    $aliases = d()->aliases;
    if (count($aliases)) {
      foreach ($aliases as $alias) {
        if (strpos($alias, '/')) {
          drush_log("Subdirectory alias `$alias` found. Creating configuration files.",'warning');
          _subdirs_create_site_location($alias);
          _subdirs_create_domain_vhost($alias);
        }
      }
    }
  }
}

/**
 * Implements drush_hook_pre_provision_install_rollback().
 */
function drush_subdirs_pre_provision_install_rollback() {
  $aliases = d()->aliases;
  if (count($aliases)) {
    foreach ($aliases as $alias) {
      _subdirs_delete_site_location($alias);
      _subdirs_delete_domain_vhost($alias);
    }
  }
}

/**
 * Create the config file for the subdirectory.
 */
function _subdirs_create_site_location($alias) {
  $alias = explode('/', $alias, 2);
  $context = d();

  // Generate variables to pass to template
  $variables = array('db_type', 'db_name', 'db_user', 'db_passwd', 'db_host', 'db_port');
  foreach ($variables as $key => $value) {
    $variables[$value] = drush_get_option($value, NULL, 'site');
    unset($variables[$key]);
  }
  $variables['site_path'] = $context->site_path;
  $variables['platform_path'] = $context->platform->root;
  $variables['location'] = $alias[1];
  $output = _subdirs_render_template('site_location.tpl.php', $variables);

  // Create the relevant include paths
  $subdirs_path = $context->server->http_app_path . '/subdirs.d';
  if (!is_dir($subdirs_path)) {
    mkdir($subdirs_path);
  }
  $includes_path = $subdirs_path . '/' . $alias[0];
  if (!is_dir($includes_path)) {
    mkdir($includes_path);
  }

  $location_path = $includes_path . '/' . $alias[1] . '.conf';

  drush_log('Creating site location config file for subdirectory ' . $alias. ' at ' . $location_path, 'warning');

  return fwrite(fopen($location_path, 'w'), $output);
}

/**
 * Create the vhost for the subdirectory domain.
 */
function _subdirs_create_domain_vhost($alias) {
  $alias = explode('/', $alias, 2);
  $context = d();

  // Generate variables to pass to template
  $variables = array(
    'http_port' => $context->server->http_port,
    'subdirs_path' => $context->server->http_app_path . '/subdirs.d',
    'uri' => $alias[0],
  );

  $output = _subdirs_render_template('domain_vhost.tpl.php', $variables);

  $domain_vhost_path = $context->server->http_vhostd_path . '/' . $alias[0];

  drush_log('Creating domain vhost for subdirectory ' . $alias . ' at ' . $domain_vhost_path, 'warning');

  return fwrite(fopen($domain_vhost_path, 'w'), $output);
}

/**
 * Delete the config file for the subdirectory.
 */
function _subdirs_delete_site_location($alias) {
  drush_log('Deleting site location config file for subdirectory ' . $alias, 'warning');
}

/**
 * Delete the vhost for the subdirectory domain.
 */
function _subdirs_delete_domain_vhost($alias) {
  drush_log('Deleting domain vhost for subdirectory ' . $alias, 'warning');
}



// TODO: Remove this, once we've got a working Config class.
function _subdirs_render_template($template, $variables) {

  $template_path = dirname(__FILE__) . '/Provision/Config/' . $template;

  extract($variables);
  ob_start();
  include $template_path;
  $content = ob_get_clean();

  return $content;
}

