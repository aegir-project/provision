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
    drush_log('setting site_data property on {$context->name} context to ' . $context->site_data);
    $context->setProperty('site_data');
  }
}
