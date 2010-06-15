<?php

class provisionService_http_cluster extends provisionService_http {
  static function option_documentation() {
    return array(
      '--cluster_web_servers' => 'server with cluster: comma-separated list of web servers.'
    );
  }

  function init() {
    $this->server->setProperty('cluster_web_servers', array(), TRUE);
  }

  function setContext($name) {
    parent::setContext($name);
    foreach ($this->server->cluster_web_servers as $server) {
      call_user_func_array(array(d($server)->service('http'), 'setContext'), array($name));
    }
  }

  function _each_server($method, $args = array()) {
    foreach ($this->server->cluster_web_servers as $server) {
      call_user_func_array(array(d($server)->service('http', $this->context), $method), $args);
    }
  }

  function parse_configs() {
    $this->_each_server(__FUNCTION__);
  }

  function create_site_config() {
    $this->_each_server(__FUNCTION__);
  }

  function delete_site_config() {
    $this->_each_server(__FUNCTION__);
  }

  function create_platform_config() {
    $this->_each_server(__FUNCTION__);
  }

  function delete_platform_config() {
    $this->_each_server(__FUNCTION__);
  }

  function create_server_config() {
    $this->_each_server(__FUNCTION__);
  }

  function delete_server_config() {
    $this->_each_server(__FUNCTION__);
  }

  function sync() {
    $args = func_get_args();
    $this->_each_server(__FUNCTION__, $args);
  }
}
