<?php

function drush_provision_bind_provision_zone($origin) {
  provision_bind_update_zone((object) drush_get_option('zone'));
  _provision_bind_restart($origin);
}
