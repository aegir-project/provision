<?php

// Base http service class. 
class Provision_Service_http extends Provision_Service {
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
