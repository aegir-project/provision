<?php

include_once(dirname(__FILE__) . '/../provision.service.inc');

function http_provision_services() {
  return array('http' => NULL);
}


// Base http service class. 
class provisionService_http extends provisionService {
  public $service = 'http';
  protected $ssl_enabled = FALSE;


  /**
   * Support the ability to cloak the database credentials using environment variables.
   */
  function cloaked_db_creds() {
    return FALSE;
  }


  function verify_server_cmd() {
    $this->create_config($this->context->type);
    $this->parse_configs();
  }

  function verify_platform_cmd() {
    $this->create_config($this->context->type);
    $this->parse_configs();
  }

  function verify_site_cmd() {
    $this->create_config($this->context->type);
    $this->parse_configs();
  }


  /**
   * Register the http handler for platforms, based on the web_server option.
   */
  static function subscribe_platform($context) {
    $context->setProperty('web_server', '@server_master');
    $context->is_oid('web_server');
    $context->service_subscribe('http', $context->web_server->name);
  }

}

// Public http service , as in non-encrypted and listening on a port.
class provisionService_http_public extends provisionService_http {
  protected $has_port = TRUE;
   
  function default_port() {
    return 80;
  }

  function config_data($config = null, $class = null) {
    $data = parent::config_data($config, $class);
    if (!is_null($this->application_name)) {
      $data['http_pred_path'] = $this->server->http_pred_path;
      $data['http_postd_path'] = $this->server->http_postd_path;
      $data['http_platformd_path'] = $this->server->http_platformd_path;
      $data['http_vhostd_path'] = $this->server->http_vhostd_path;
    }

    $data['http_port'] = $this->server->http_port;

    // We assign this generic catch all for standard http.
    // The SSL based services will override this with the
    // correct ip address.
    if (sizeof($this->server->ip_addresses)) {
      // Use the first IP address for all standard virtual hosts.
      $data['ip_address'] = $this->server->ip_addresses[0];
    }
    else {
      // If no external ip addresses are defined, we fall back on *:port
      // There will be no SSL , so that's fine.
      $data['ip_address'] = '*';
    }

    // TODO: move away from drush_get_context entirely.
    if ($config == 'site') {

      // DO not create it with the port here. Protocol only is enough.
      $data['redirect_url'] = "http://{$this->context->uri}";

      $data = array_merge($data, drush_get_context('site'));
    }

    return $data;
  }

  function init_server() {
    parent::init_server();
    // System account
    if ($this->server->name == '@server_master') {
      $this->server->setProperty('web_group', _provision_default_web_group());
    }
    else {
      $this->server->web_group = d('@server_master')->web_group;
    }


    // Redirection urls
    $this->server->web_disable_url = rtrim($this->server->master_url, '/') .'/hosting/disabled';
    $this->server->web_maintenance_url = rtrim($this->server->master_url, '/') .'/hosting/maintenance';


    if (!is_null($this->application_name)) {
      $app_dir = "{$this->server->config_path}/{$this->application_name}";
      $this->server->http_pred_path = "{$app_dir}/pre.d";
      $this->server->http_postd_path = "{$app_dir}/post.d";
      $this->server->http_platformd_path = "{$app_dir}/platform.d";
      $this->server->http_vhostd_path = "{$app_dir}/vhost.d";
      $this->server->http_platforms_path = "{$this->server->aegir_root}/platforms";
    }
  }


  static function option_documentation() {
    return array(
      '--web_group' => 'server with http: OS group for permissions; working default will be attempted',
      '--web_disable_url' => 'server with http: URL disabled sites are redirected to; default {master_url}/hosting/disabled',
      '--web_maintenance_url' => 'server with http: URL maintenance sites are redirected to; default {master_url}/hosting/maintenance',
    );
  }


  function verify_server_cmd() {
    if (!is_null($this->application_name)) {
      provision_file()->create_dir($this->server->http_pred_path, dt("Webserver custom pre-configuration"), 0700);
      $this->sync($this->server->http_pred_path);
      provision_file()->create_dir($this->server->http_postd_path, dt("Webserver custom post-configuration"), 0700);
      $this->sync($this->server->http_postd_path);

      provision_file()->create_dir($this->server->http_platformd_path, dt("Webserver platform configuration"), 0700);
      $this->sync($this->server->http_platformd_path, array(
        'exclude' => $this->server->http_platformd_path . '/*',  // Make sure remote directory is created
      )); 

      provision_file()->create_dir($this->server->http_vhostd_path , dt("Webserver virtual host configuration"), 0700);
      $this->sync($this->server->http_vhostd_path, array(
        'exclude' => $this->server->http_vhostd_path . '/*',  // Make sure remote directory is created
      ));

      provision_file()->create_dir($this->server->http_platforms_path, dt("Platforms"), 0755);
      $this->sync($this->server->http_platforms_path, array(
        'exclude' => $this->server->http_platforms_path . '/*',  // Make sure remote directory is created
      ));
    } 

    parent::verify_server_cmd();
  }

  /**
   * Ask the web server to check for and load configuration changes.
   */
  function parse_configs() {
    return TRUE;
  }

  /**
   * Return a list of servers that will need database access.
   */
  function grant_server_list() {
    return array(
      $this->server,
      $this->context->platform->server,
    );
  }
}


include_once('http.config.inc');
include_once('http.ssl.inc');
