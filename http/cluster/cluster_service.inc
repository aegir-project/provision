<?php

class provisionService_http_cluster extends provisionService_http {
  private $servers = array();

  static function option_documentation() {
    return array(
      '--cluster_web_servers' => 'server with cluster: comma-separated list of web servers.'
    );
  }

  function init() {
    $this->setProperty('cluster_web_servers', array(), TRUE);
    foreach ($this->cluster_web_servers as $server) {
      $this->servers[] = d($server);
    }
  }

  function parse_configs() {
    foreach ($this->servers as $server) {
      $server->service('http')->parse_configs();
    }
  }

  function create_site_config() {
    foreach ($this->servers as $server) {
      $server->service('http')->create_site_config();
    }
  }

  function delete_site_config() {
    foreach ($this->servers as $server) {
      $server->service('http')->delete_site_config();
    }
  }

  function create_platform_config() {
    foreach ($this->servers as $server) {
      $server->service('http')->create_platform_config();
    }
  }

  function delete_platform_config() {
    foreach ($this->servers as $server) {
      $server->service('http')->delete_platform_config();
    }
  }

  function create_server_config() {
    foreach ($this->servers as $server) {
      $server->service('http')->create_server_config();
    }
  }

  function delete_server_config() {
    foreach ($this->servers as $server) {
      $server->service('http')->delete_server_config();
    }
  }
}
