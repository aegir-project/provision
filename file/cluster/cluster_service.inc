<?php

class provisionService_file_cluster extends provisionService_file {
  function sync($path = NULL, $exclude_sites = FALSE) {
    foreach ($this->cluster_web_servers as $server) {
      d($server)->service('file')->sync($path, $exclude_sites);
    }
  }
}
