server {
  listen       *:<?php print $http_port; ?>;
  server_name  <?php print $this->uri . ' ' . implode(' ', str_replace('/', '.', $this->aliases)); ?>;
  return       404;
  ### Do not reveal Aegir front-end URL here.
}
