<?php

function drush_provision_bind_provision_zone($zone) {
  provision_bind_update_zone(drush_get_option('zone'));
  _provision_bind_restart();
}
