server {
  listen       *:<?php print $http_port; ?>;
  server_name  <?php print $this->uri . ' ' . implode(' ', $this->aliases); ?>;
  return       404;
  ### Dont't reveal Aegir front-end URL here.
}
