<?php
/**
 * @file Site.php
 *
 *       Apache Configuration for Server Context.
 * @see \Provision_Config_Apache_Site
 * @see \Provision_Config_Http_Site
 */

namespace Aegir\Provision\Service\Http\Apache\Configuration;

use Aegir\Provision\Configuration;

class SiteConfiguration extends Configuration {
  
  const SERVICE_TYPE = 'apache';
  
  public $template = 'vhost.tpl.php';
  // The template file to use when the site has been disabled.
  public $disabled_template = 'vhost_disabled.tpl.php';
  public $description = 'virtual host configuration file';
  
  
  function filename() {
      $file = $this->context->getProperty('uri') . '.conf';
      return $this->context->getProvision()->getConfig()->get('config_path') . '/' . $this->service->provider->name . '/' . $this->service->getType() . '/vhost.d/' . $file;
  }
  
  function process() {
      parent::process();
      $this->data['http_port'] = $this->context->getSubscription('http')->service->getProperty('http_port');
      $this->data['root'] = $this->context->platform->getProperty('root');
      $this->data['uri'] = $this->context->getProperty('uri');

      $this->data['site_path'] = $this->data['root'] . '/sites/' . $this->data['uri'];

      $this->data['db_type'] = $this->context->getSubscription('db')->service->getType();

//      print_r($this->context->getSubscription('db'));

      $this->data['db_name'] = $this->context->getSubscription('db')->getProperty('db_name');
      $this->data['db_user'] = $this->context->getSubscription('db')->getProperty('db_user');
      $this->data['db_passwd'] = $this->context->getSubscription('db')->getProperty('db_password');
      $this->data['db_host'] = $this->context->getSubscription('db')->service->provider->getProperty('remote_host');

      $this->data['db_port'] = $this->context->getSubscription('db')->service->getCreds()['port'];

      $this->data['extra_config'] = '';

//    if ($this->aliases && !is_array($this->aliases)) {
//      $this->aliases = explode(",", $this->aliases);
//    }
//
//    $this->aliases = array_filter($this->aliases, 'trim');
//
//    if ($this->drush_aliases && !is_array($this->drush_aliases)) {
//      $this->drush_aliases = explode(",", $this->drush_aliases);
//    }
//
//    $this->drush_aliases = array_filter($this->drush_aliases, 'trim');
//
//    if (!$this->site_enabled) {
//      $this->template = $this->disabled_template;
//    }
//
//    $app_dir = $this->context->application->getConfig()->get('config_path') . '/' . $this->service->getType();
//
////    $this->data['http_port'] = $this->service->properties['http_port'];
////    $this->data['include_statement'] = '# INCLUDE STATEMENT';
////    $this->data['http_pred_path'] = "{$app_dir}/pre.d";
////    $this->data['http_postd_path'] = "{$app_dir}/post.d";
////    $this->data['http_platformd_path'] = "{$app_dir}/platform.d";
////    $this->data['extra_config'] = "";
//
//    $this->data['http_vhostd_path'] = "{$app_dir}/vhost.d";
//
  }
}