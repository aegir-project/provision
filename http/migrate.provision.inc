<?php

function drush_http_pre_provision_migrate() {
  d()->service('http')->delete_config('site'); 
}

function drush_http_pre_provision_migrate_rollback() {
  d()->service('http')->create_config('site');
}
