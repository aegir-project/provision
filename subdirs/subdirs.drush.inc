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
  _subdirs_create_symlinks();
}

/**
 * Implements drush_hook_pre_provision_install_rollback().
 */
function drush_subdirs_pre_provision_install_rollback() {
  _subdirs_remove_symlinks();
}

/**
 * Implementation of drush_hook_pre_provision_verify()
 */
function drush_subdirs_pre_provision_verify() {
  _subdirs_create_symlinks();
}

/**
 * Implementation of drush_hook_pre_provision_deploy()
 */
function drush_subdirs_pre_provision_deploy() {
  _subdirs_create_symlinks();
}

/**
 * Implementation of drush_hook_pre_provision_delete()
 */
function drush_subdirs_pre_provision_delete() {
  _subdirs_remove_symlinks();
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
 * Create subdirectory symlinks
 *
 * This create a sites/example.com.foo symlink pointing to the main
 * URL of the site.
 */
function _subdirs_create_symlinks() {
  if (d()->type == 'site') {
    $aliases = d()->aliases;
    if (count($aliases)) {
      foreach ($aliases as $alias) {
        if (strpos($alias, '/')) {
          drush_log("Subdirectory alias `$alias` found. Creating magic symlinks.", 'notice');
          provision_file()->symlink(d()->uri, 'sites/' . str_replace('/', '.', $alias))
            ->succeed('symlinked @path to @target')
            ->fail('failed to symlink @path to @target');
        }
      }
    }
  }
}

/**
 * Delete subdirectory symlinks
 *
 * This deletes the symlinks created by _subdirs_create_symlinks().
 */
function _subdirs_remove_symlinks() {
  if (d()->type == 'site') {
    $aliases = d()->aliases;
    if (count($aliases)) {
      foreach ($aliases as $alias) {
        provision_file()->unlink('sites/' . str_replace('/', '.', $alias))
          ->succeed('removed @path symlink')
          ->fail('failed to remove @path symlink');
      }
    }
  }
}
