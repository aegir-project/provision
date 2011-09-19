<?php
global $databases;
if ($db = $databases['default']['default']) {
  drush_set_option('db_type', urldecode($db['driver']), 'site');
  drush_set_option('db_user', urldecode($db['username']), 'site');
  drush_set_option('db_host', urldecode($db['host']), 'site');
  drush_set_option('db_passwd', urldecode($db['password']), 'site');
  drush_set_option('db_name', urldecode($db['database']), 'site');

  drush_set_option('profile', variable_get('install_profile', 'default'), 'site');
  $language = language_default();
  drush_set_option('language', $language->language, 'site');
}
