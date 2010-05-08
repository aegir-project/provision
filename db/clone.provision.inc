<?php

// $Id$
function drush_db_provision_clone_validate() {
  provision_service('db')->connect();
}
