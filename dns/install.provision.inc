<?php

// $Id: install.provision.inc,v 1.7 2009/05/07 22:04:30 adrian Exp $

/**
 * @file
 *   Provision hooks for the install command
 **/

function drush_dns_post_provision_install() {
 // we create this post-install because we need the ip's to be assigned already.
 d()->service('dns')->create_host();
 d()->service('dns')->parse_configs();
}
