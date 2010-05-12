<?php

function drush_http_provision_verify() {
  provision_service('http')->verify();
}
