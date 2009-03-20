<?php

function provision_apache_provision_delete($url = NULL) {
  _provision_apache_delete_vhost_config($url);
  return _provision_apache_restart_apache();  
}

