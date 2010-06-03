<?php

// $Id$

function drush_provision_ssl_post_provision_verify() {
  if (d()->type === 'site') {
    drush_set_option('ssl', drush_get_option('ssl'), 'site');
  }
}
