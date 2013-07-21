<VirtualHost *:<?php print $http_port; ?>>

  ServerName <?php print $uri; ?>

  Include <?php print $subdirs_path; ?>/<?php print $uri; ?>/*.conf

</VirtualHost>

