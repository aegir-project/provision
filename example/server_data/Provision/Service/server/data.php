<?php

/**
 * The server_data service class.
 */
class Provision_Service_server_data extends Provision_Service {
  public $service = 'server_data';

  /**
   * Add the needed properties to the server context.
   */
  static function subscribe_server($context) {
    $context->setProperty('server_data');
  }
}
