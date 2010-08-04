<?php

// $Id: delete.provision.inc,v 1.3 2009/05/07 22:04:30 adrian Exp $

/**
 * @file
 *   Provision hooks for the delete command
 **/

function drush_dns_post_provision_delete() {
  if (d()->type == 'site') {
    d()->service('dns')->delete_host();
    d()->service('dns')->parse_configs();
  }
}
