<?php

function provision_apache_provision_verify() {
  _provision_create_dir(PROVISION_VHOST_PATH, dt("Virtual host configuration"), 0700);
  _provision_apache_restart_apache(TRUE);
}

