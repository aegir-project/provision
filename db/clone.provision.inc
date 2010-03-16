<?php

// $Id$
function drush_db_provision_clone_validate($url = null) {
  provision_service('db')->connect($url);
}
