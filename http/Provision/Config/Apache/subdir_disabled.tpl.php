<Location /<?php print $subdir; ?>>

  RewriteEngine on
  # the ? at the end is to remove any query string in the original url
  RewriteRule ^(.*)$ <?php print $web_disable_url . '/' . $uri ?>?

</Location>


