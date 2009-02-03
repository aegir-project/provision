<?php

function provision_mysql_provision_install_validate() {
  provision_db_connect();
}

function provision_mysql_provision_pre_install(&$data, $url = NULL) {
  $data['db_type'] = ($data['db_type']) ? $data['db_type'] : PROVISION_DB_TYPE;
  $data['db_host'] = ($data['db_host']) ? $data['db_host'] : PROVISION_DB_HOST;

  # generate a random password for use
  $data['db_passwd'] = provision_password(); 
  $data['db_name'] = _provision_mysql_suggest_db_name($data, $url);
  $data['db_user'] = $data['db_name']; 

  _provision_mysql_new_site_db($data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);
}

function provision_mysql_provision_pre_install_rollback(&$data, $url = NULL) {
  _provision_mysql_destroy_site_db($data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);
  unset($data['db_host']);
  unset($data['db_type']);
  unset($data['db_passwd']);
  unset($data['db_user']);
  unset($data['db_name']);
}
