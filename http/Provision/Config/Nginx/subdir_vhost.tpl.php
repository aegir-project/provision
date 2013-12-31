server {
  limit_conn    gulag 32; # like mod_evasive - this allows max 32 simultaneous connections from one IP address
  listen        *:<?php print $http_port; ?>;
  server_name   <?php print $uri; ?>;
  include       <?php print $subdirs_path; ?>/<?php print $uri; ?>/*.conf;
}
