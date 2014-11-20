<?php

/**
 * The site_data service class.
 */
class Provision_Service_site_data extends Provision_Service {
  public $service = 'site_data';

  /**
   * Add the needed properties to the site context.
   */
  static function subscribe_site($context) {
    $context->setProperty('site_data');
  }
}
