<?php

function provision_apache_provision_delete(&$data, $url = NULL) {
  _provision_apache_delete_vhost_config($url, $data);
  return _provision_apache_restart_apache();  
}

