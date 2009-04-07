<?php

if ($parts = @parse_url($GLOBALS['db_url'])) {
  drush_set_option('db_type', $parts['scheme'], 'site');
  drush_set_option('db_user', $parts['user'], 'site');
  drush_set_option('db_host', $parts['host'], 'site');
  drush_set_option('db_passwd', $parts['pass'], 'site');
  drush_set_option('db_name', substr($parts['path'], 1), 'site');

  drush_set_option('profile', variable_get('install_profile', 'default'), 'site');
  $language = language_default();
  drush_set_option('language', $language->language, 'site');
}
