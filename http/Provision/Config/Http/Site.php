<?php

/**
 * Base class for virtual host configuration files.
 */
class Provision_Config_Http_Site extends Provision_Config_Http {
  public $template = 'vhost.tpl.php';
  // The template file to use when the site has been disabled.
  public $disabled_template = 'vhost_disabled.tpl.php';
  public $description = 'virtual host configuration file';


  function filename() {
    return $this->data['http_vhostd_path'] . '/' . $this->uri;
  }

  function write() {
    parent::write();

    // We also leave a record of this IP in the site's drushrc.php
    // This way we can pass the info back to the front end.
    $ip_addresses = drush_get_option('site_ip_addresses', array(), 'site');

    if ($this->data['ip_address'] != '*') {
      $ip_addresses[$this->data['server']->name] = $this->data['ip_address'];
    }
    elseif (isset($context['site_ip_addresses'][$this->data['server']->name])) {
      unset($ip_addresses[$this->data['server']->name]);
    }
    drush_set_option('site_ip_addresses', $ip_addresses, 'site');
  }

  function unlink() {
    parent::unlink();

    // We also remove the record of this IP in the site's drushrc.php
    // This way we can pass the info back to the front end.
    $ip_addresses = drush_get_option('site_ip_addresses', array(), 'site');
    unset($ip_addresses[$this->data['server']->name]);
    drush_set_option('site_ip_addresses', $ip_addresses, 'site');
  }

  function process() {
    parent::process();

    if ($this->aliases && !is_array($this->aliases)) {
      $this->aliases = explode(",", $this->aliases);
    }

    $this->aliases = array_filter($this->aliases, 'trim');

    if (!$this->site_enabled) {
      $this->template = $this->disabled_template;
    }

  }
}
