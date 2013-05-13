<?php

/**
 * Nginx specific config includes.
 */
class Provision_Config_Nginx_Inc_Server extends Provision_Config_Http_Inc_Server {
  // We use the same extra_config as the nginx_server config class.
  function process() {
    parent::process();
  }
}
