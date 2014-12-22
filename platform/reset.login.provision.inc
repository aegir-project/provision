<?php
/**
 * @file
 *   Provision hooks for the provision login_reset command.
 */

/**
 * Provision login-reset command
 *
 * These are the hooks that will be executed by the drush_invoke function
 * when generating a one-time login reset URL.
 */

/**
 * Generate a one-time login reset url
 */
function drush_provision_drupal_provision_login_reset() {
  // pass the login link to the front end
  drush_set_option('browser', FALSE);
  $onetime = drush_user_login();
  drush_set_option('login_link', $onetime);
  drush_log(dt('Login url: !onetime', array('!onetime' => $onetime)), 'success');
}
