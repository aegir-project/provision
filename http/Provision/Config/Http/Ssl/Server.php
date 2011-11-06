<?php

/**
 * Base class for SSL enabled server level config.
 */
class Provision_Config_Http_Ssl_Server extends Provision_Config_Http_Server {
  public $template = 'server_ssl.tpl.php';
  public $description = 'encryption enabled webserver configuration';
}
