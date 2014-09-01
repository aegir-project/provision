<?php

/**
 * @file
 *   Installation settings common across different versions of Drupal.
 */

define('PROVISION_DEFAULT_ADMIN_USER', 'admin');

/**
 * Validate client email and throw a helpful error message if it is invalid.
 */
function install_validate_client_email($client_email) {
  if (!$client_email || !filter_var($client_email, FILTER_VALIDATE_EMAIL)) {
    $invalid_email = 'this.email@is.invalid';
    $user = function_exists('l') ? l(dt('user account'), 'user') : 'user account';
    drush_log(dt('Error validating client email `!email`. Please check your !user email. To allow the site installation to proceed, the email `!invalid` has been used instead. Please use the `login` link to change the site admin email to a proper valid address.',
      array(
        '!email' => $client_email,
        '!user' => $user,
        '!invalid' => $invalid_email,
      )),
      'warning'
    );
    return $invalid_email;
  }
  else {
    return $client_email;
  }
}
