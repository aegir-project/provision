<?php
global $databases;
if ($db = $databases['default']['default']) {
  $data['db_type'] = $db['driver'];
  $data['db_user'] = $db['username'];
  $data['db_host'] = $db['host'];
  $data['db_passwd'] = $db['password'];
  $data['db_name'] = $db['database'];

  $data['profile'] = variable_get('install_profile', 'default');
  $language = language_default();
  $data['language'] = $language->language;
}
