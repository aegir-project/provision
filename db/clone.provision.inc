<?php

// $Id$
function drush_db_provision_clone_validate() {
  d()->service('db')->connect();
}
