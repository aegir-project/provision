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
    #drush_print_r($context->options);
    drush_print_r(drush_get_context('stdin'));
    $context->setProperty('server_data');
  }
}
