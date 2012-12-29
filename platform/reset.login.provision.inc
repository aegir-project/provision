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
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
  // Drupal 7/8 take a uid as an integer and not an array
  if (drush_drupal_major_version() >= 7) {
    $account = user_load(1);
  }
  else {
    $account = user_load(array('uid' => 1));
  }
  if (empty($account)) {
    return drush_set_error('PROVISION_UNABLE_TO_LOAD_UID_1', 'Could not load the admin user with uid 1 on this site.');
  }
  $onetime = user_pass_reset_url($account) . '/login';
  // pass the login link to the front end
  drush_set_option('login_link', $onetime);
  drush_log(t('Login url: !onetime', array('!onetime' => $onetime)), 'message');

  if (drush_drupal_major_version() >= 7) {
    drupal_session_destroy_uid(1);
  }
  else {
    // valid for d5 and d6
    sess_destroy_uid(1);
  }
}
