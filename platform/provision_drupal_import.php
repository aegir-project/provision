<?php

require_once(dirname(__FILE__) . '/../provision.inc');
if ($argv[1]) {
  $data = provision_external_init($argv[1]);
}
else {
  provision_set_error(PROVISION_FRAMEWORK_ERROR);
  provision_log("error", "USAGE: import.php url\n");
}

if (is_array($GLOBALS['db_url'])) {
  $db_url = $GLOBALS['db_url']['default'];
}

if ($parts = @parse_url($db_url)) {
  $data['db_type'] = $parts['scheme'];
  $data['db_user'] = $parts['user'];
  $data['db_host'] = $parts['host'];
  $data['db_passwd'] = $parts['pass'];
  $data['db_name'] = substr($parts['path'], 1);

  $data['profile'] = variable_get('install_profile', 'default');
  $language = language_default();
  $data['language'] = $language->language;
}
provision_output($argv[1], $data);

print(serialize($data));
exit(PROVISION_SUCCESS);
