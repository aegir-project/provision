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
  _subdirs_create_site_configs();
}

/**
 * Implements drush_hook_pre_provision_install_rollback().
 */
function drush_subdirs_pre_provision_install_rollback() {
  _subdirs_remove_site_configs();
}

/**
 * Implementation of drush_hook_pre_provision_verify()
 */
function drush_subdirs_pre_provision_verify() {
  _subdirs_create_site_configs();
}

/**
 * Implementation of drush_hook_post_provision_deploy()
 */
function drush_subdirs_pre_provision_deploy() {
  _subdirs_create_site_configs();
}

/**
 * Implementation of drush_hook_provision_delete()
 */
function drush_subdirs_pre_provision_delete() {
  _subdirs_remove_site_configs();
}

/**
 * Implementation of drush_hook_provision_disable()
 */
function drush_subdirs_pre_provision_disable() {
  _subdirs_disable_sites();
}

/**
 * Implements drush_hook_pre_provision_enable().
 */
function drush_subdirs_pre_provision_enable() {
  _subdirs_create_site_configs();
}

/**
 * Implements hook_provision_config_load_templates().
 */
function subdirs_provision_config_load_templates($config) {
  if ($config->template == 'provision_drupal_sites.tpl.php') {
    $context = d();
    $use_subdirs_tpl = FALSE;
    foreach ($context->aliases as $alias) {
      if (strpos($alias, '/')) {
        $use_subdirs_tpl = TRUE;
      }
    }
    if ($use_subdirs_tpl) {
      drush_log('Generating sites.php for use with subdirectories.', 'notice');
      return dirname(__FILE__) . '/Provision/Config/subdirs_drupal_sites.tpl.php';
    }
  }
}

/**
 * Helper function to create configuration files for subdirectories.
 */
function _subdirs_create_site_configs() {
  if (d()->type == 'site') {
    $aliases = d()->aliases;
    if (count($aliases)) {
      foreach ($aliases as $alias) {
        if (strpos($alias, '/')) {
          drush_log("Subdirectory alias `$alias` found. Creating configuration files.", 'notice');
          _subdirs_create_site_location($alias);
          _subdirs_create_domain_vhost($alias);
        }
      }
    }
  }
}

/**
 * Helper function to remove configuration files for subdirectories.
 */
function _subdirs_remove_site_configs() {
  $context = d();
  $aliases = $context->aliases;
  $subdirs_path = $context->server->http_app_path . '/subdirs.d';
  if (count($aliases)) {
    foreach ($aliases as $alias) {
      _subdirs_delete_site_location($alias);
      // We only want to remove the domain vhost after the last subdirectory
      // site has been deleted.
      $alias_parts = explode('/', $alias, 2);
      $includes_path = $subdirs_path . '/' . $alias_parts[0];
      if ($handle = opendir($includes_path)) {
        if (FALSE === readdir($handle)) {
          _subdirs_delete_domain_vhost($alias);
        }
      }
    }
  }
}

/**
 * Create the config file for the subdirectory.
 */
function _subdirs_create_site_location($alias) {
  $alias_parts = explode('/', $alias, 2);
  $context = d();

  // Generate variables to pass to template
  $variables = array('db_type', 'db_name', 'db_user', 'db_passwd', 'db_host', 'db_port');
  foreach ($variables as $key => $value) {
    $variables[$value] = drush_get_option($value, NULL, 'site');
    unset($variables[$key]);
  }
  $variables['site_path'] = $context->site_path;
  $variables['platform_path'] = $context->platform->root;
  $variables['location'] = $alias_parts[1];
  $output = _subdirs_render_template('site_location.tpl.php', $variables);

  // Create the relevant include paths
  $subdirs_path = $context->server->http_app_path . '/subdirs.d';
  if (!is_dir($subdirs_path)) {
    mkdir($subdirs_path);
  }
  $includes_path = $subdirs_path . '/' . $alias_parts[0];
  if (!is_dir($includes_path)) {
    mkdir($includes_path);
  }

  $location_path = $includes_path . '/' . $alias_parts[1] . '.conf';

  if (fwrite(fopen($location_path, 'w'), $output)) {
    drush_log('Created site location config file for subdirectory ' . $alias . ' at ' . $location_path, 'success');
  }
  else {
    drush_log('Error encountered attempting to create site location config file for subdirectory ' . $alias, 'error');
  }

}

/**
 * Create the vhost for the subdirectory domain.
 */
function _subdirs_create_domain_vhost($alias) {
  $alias_parts = explode('/', $alias, 2);
  $context = d();

  // Generate variables to pass to template
  $variables = array(
    'http_port' => $context->server->http_port,
    'subdirs_path' => $context->server->http_app_path . '/subdirs.d',
    'uri' => $alias_parts[0],
  );

  $output = _subdirs_render_template('domain_vhost.tpl.php', $variables);

  $domain_vhost_path = $context->server->http_vhostd_path . '/' . $alias_parts[0];

  if (fwrite(fopen($domain_vhost_path, 'w'), $output)) {
    drush_log('Created domain vhost for subdirectory ' . $alias . ' at ' . $domain_vhost_path, 'success');
  }
  else {
    drush_log('Error encountered attempting to create domain vhost for subdirectory ' . $alias, 'error');
  }

}

/**
 * Helper function to remove configuration files for subdirectories.
 */
function _subdirs_disable_sites() {
  $aliases = d()->aliases;
  if (count($aliases)) {
    foreach ($aliases as $alias) {
      if (strpos($alias, '/')) {
        drush_log("Disabling subdirectory site $alias.", 'notice');
        _subdirs_create_disabled_site_location($alias);
      }
    }
  }
}

/**
 * Create the config file for the disabled subdirectory.
 */
function _subdirs_create_disabled_site_location($alias) {
  $alias_parts = explode('/', $alias, 2);
  $context = d();

  // Generate variables to pass to template
  $variables['location'] = $alias_parts[1];
  $variables['web_disable_url'] = $context->platform->server->web_disable_url;
  $variables['uri'] = $context->uri;
  $output = _subdirs_render_template('site_location_disabled.tpl.php', $variables);

  $subdirs_path = $context->server->http_app_path . '/subdirs.d';
  $includes_path = $subdirs_path . '/' . $alias_parts[0];
  $location_path = $includes_path . '/' . $alias_parts[1] . '.conf';

  if (fwrite(fopen($location_path, 'w'), $output)) {
    drush_log('Created disabled site location config file for subdirectory ' . $alias . ' at ' . $location_path, 'success');
  }
  else {
    drush_log('Error encountered attempting to create disabled site location config file for subdirectory ' . $alias, 'error');
  }

}

/**
 * Delete the config file for the subdirectory.
 */
function _subdirs_delete_site_location($alias) {
  $alias_parts = explode('/', $alias, 2);
  $context = d();

  $subdirs_path = $context->server->http_app_path . '/subdirs.d';
  $includes_path = $subdirs_path . '/' . $alias_parts[0];
  $location_path = $includes_path . '/' . $alias_parts[1] . '.conf';

  if (unlink($location_path)) {
    drush_log('Deleted site location config file for subdirectory ' . $alias, 'success');
  }
  else {
    drush_log('Error encountered attempting to delete site location config file for subdirectory ' . $alias, 'error');
  }

}

/**
 * Delete the vhost for the subdirectory domain.
 */
function _subdirs_delete_domain_vhost($alias) {
  $alias_parts = explode('/', $alias, 2);
  $context = d();

  $domain_vhost_path = $context->server->http_vhostd_path . '/' . $alias_parts[0];

  if (unlink($domain_vhost_path)) {
    drush_log('Deleted domain vhost for subdirectory ' . $alias, 'success');
  }
  else {
    drush_log('Error encountered attempting to delete domain vhost for subdirectory ' . $alias, 'error');
  }

}

/**
 * Render our config file templates.
 */
function _subdirs_render_template($template, $variables) {
  // TODO: Remove this, once we've got a working Config class.

  $template_path = dirname(__FILE__) . '/Provision/Config/' . $template;

  extract($variables);
  ob_start();
  include $template_path;
  $content = ob_get_clean();

  return $content;
}

