<?php
 // $Id$

/**
 * @file
 *    Configuration classes for the HTTP service.
 */


/**
 * Base class for HTTP config files.
 *
 * This class will publish the config files to remote
 * servers automatically.
 */
class provisionConfig_http extends provisionConfig {
  function write() {
    parent::write();
    $this->data['server']->sync($this->filename());
  }

  function unlink() {
    parent::unlink();
    $this->data['server']->sync($this->filename());
  }
}

/**
 * Base configuration class for server level http configs.
 *
 * This class uses the service defined application_name, to generate
 * a top level $app_name.conf for the service.
 *
 * Each server has it's own configuration, and this class will
 * automatically generate a symlink to the correct file for each
 * server.
 */
class provisionConfig_http_server extends provisionConfig_http {
  public $template = 'server.tpl.php';
  public $description = 'web server configuration file';

  function write() {
    parent::write();

    if (isset($this->data['application_name'])) {
      $file = $this->data['application_name'] . '.conf';
      // We link the app_name.conf file on the remote server to the right version.
      $cmd = sprintf('ln -sf %s %s', 
        escapeshellarg($this->data['server']->config_path . '/' . $file), 
        escapeshellarg($this->data['server']->aegir_root . '/config/' . $file)
      );
      
      if ($this->data['server']->shell_exec($cmd)) {
        drush_log(dt("Created symlink for %file on %server", array(
          '%file' => $file,
          '%server' => $this->data['server']->remote_host,
        )));  
       
      };
    }
  }

  function filename() {
    if (isset($this->data['application_name'])) {
      $file = $this->data['application_name'] . '.conf';
      return $this->data['server']->config_path . '/' . $file;
    }
    else {
      return FALSE;
    }
  }


}


/**
 * Base class for platform configuration files.
 */
class provisionConfig_http_platform extends provisionConfig_http {
  public $template = 'platform.tpl.php';
  public $description = 'platform configuration file';

  function filename() {
    return $this->data['http_platformd_path'] . '/' . ltrim($this->context->name, '@') . '.conf';
  }
}


/**
 * Base class for virtual host configuration files.
 */
class provisionConfig_http_site extends provisionConfig_http {
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
