<?php

function drush_provision_bind_provision_zone($action) {
  $zone = (object) drush_get_option('zone'));
  switch ($action) {
  case 'verify':
  case 'create':
    $status = provision_bind_create_zone($zone);
    break;
  case 'delete':
    $status = provision_bind_delete_zone($zone));
    break;
  case 'modify':
    drush_set_error('PROVISION_FRAMEWORK_ERROR', dt("Not implemented"));
    break;
  }
  if ($status) {
    _provision_bind_reload($zone->origin);
  }
}
