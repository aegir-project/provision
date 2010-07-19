<?php

// $Id: install.provision.inc,v 1.7 2009/05/07 22:04:30 adrian Exp $

/**
 * @file
 *   Provision hooks for the install command
 **/

function drush_dns_pre_provision_install() {
  if (d()->type == 'site') {
    d()->service('dns')->create_host();
  }
}

function drush_dns_pre_provision_install_rollback() {
  if (d()->type == 'site') {
    d()->service('dns')->delete_host();
  }
}

function drush_dns_post_provision_install() {
  if (d()->type == 'site') {
    d()->service('dns')->commit();
  }
}
