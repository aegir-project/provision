<?php

// $Id: delete.provision.inc,v 1.3 2009/05/07 22:04:30 adrian Exp $

/**
 * @file
 *   Provision hooks for the delete command
 **/

/**
 * Implementation of hook_provision_delete()
 *
 * delete the RR for this site at least, but possibly also the zone itself?
 * need to lookup the zid for the base zone of this site, and then call dns_rr with the zid and site 'name'
 * (ie: pull off the tld and second-level domain for the 'base' zone, and treat the rest as the 'name')
 */
function drush_dns_provision_delete($url = NULL) {
  if (d()->type == 'site') {
    d()->service('dns')->delete_host(d()->uri);
  }
}

function drush_dns_post_provision_delete() {
  if (d()->type == 'site') {
    d()->service('dns')->commit();
  }
}
