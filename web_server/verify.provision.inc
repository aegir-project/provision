<?php

function provision_apache_provision_verify($data, $url) {
  if (PROVISION_CONTEXT_PLATFORM) {
    _provision_create_dir(PROVISION_VHOST_PATH, dt("Virtual host configuration"), 0700);
  }
  else {
    _provision_apache_create_vhost_config($data, $url);
  }
  _provision_apache_restart_apache(TRUE);
}

