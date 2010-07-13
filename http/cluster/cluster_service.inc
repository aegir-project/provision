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

  function create_config($config) {
    $this->each_server(__FUNCTION__, array($config));
  }

  function delete_config($config) {
    $this->each_server(__FUNCTION__, array($config));
  }

  function restart() {
    $this->_each_server(__FUNCTION__);
  }

  function sync() {
    $args = func_get_args();
    $this->_each_server(__FUNCTION__, $args);
  }

  function grant_server_list() {
    return array_merge(
      array_map('d', $this->server->cluster_web_servers),
      array(d($this->context)->platform->server)
    );
  }
}
