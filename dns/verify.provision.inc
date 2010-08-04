<?php

// $Id: verify.provision.inc,v 1.5 2009/03/26 01:40:39 anarcat Exp $

/**
 * @file
 *   Provision hooks for the verify command
 **/

function drush_dns_post_provision_verify() {
  if (d()->type == 'site') {
    d()->service('dns')->create_host();
    d()->service('dns')->parse_configs();
  }
}
