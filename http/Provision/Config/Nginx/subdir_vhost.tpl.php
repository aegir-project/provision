server {
  listen        *:<?php print $http_port; ?>;
  server_name   <?php print $uri; ?>;
  include       <?php print $subdirs_path; ?>/<?php print $uri; ?>/*.conf;
}
