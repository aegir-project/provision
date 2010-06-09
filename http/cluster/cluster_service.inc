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

  function verify() {
    $this->parse_configs();
  }

  function parse_configs() {
    foreach ($this->servers as $server) {
      $server->service('http')->parse_configs();
    }
  }
}
