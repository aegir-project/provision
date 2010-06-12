<?php

class provisionService_http_cluster extends provisionService_http {
  static function option_documentation() {
    return array(
      '--cluster_web_servers' => 'server with cluster: comma-separated list of web servers.'
    );
  }

  function init() {
    $this->setProperty('cluster_web_servers', array(), TRUE);
  }

  function _each_server($method) {
    foreach ($this->cluster_web_servers as $server) {
      d($server)->service('http')->$method();
    }
  }

  function _one_server($method) {
    d($this->cluster_web_servers[0])->service('http')->$method();
  }

  function parse_configs() {
    $this->_each_server(__FUNCTION__);
  }

  function create_site_config() {
    $this->_one_server(__FUNCTION__);
  }

  function delete_site_config() {
    $this->_one_server(__FUNCTION__);
  }

  function create_platform_config() {
    $this->_one_server(__FUNCTION__);
  }

  function delete_platform_config() {
    $this->_one_server(__FUNCTION__);
  }

  function create_server_config() {
    $this->_one_server(__FUNCTION__);
  }

  function delete_server_config() {
    $this->_one_server(__FUNCTION__);
  }
}
