<VirtualHost *:<?php print $http_port; ?>>
    <?php if ($this->site_mail) : ?>
      ServerAdmin <?php  print $this->site_mail; ?> 
    <?php endif;?>
    DocumentRoot <?php print $this->root; ?> 
    
    ServerName <?php print $this->uri; ?>

    <?php if (is_array($this->aliases)) :
     foreach ($this->aliases as $alias) : ?>
       ServerAlias <?php print $alias; ?>
     <?php
       endforeach;
     endif; ?>

    RewriteEngine on
    # the ? at the end is to remove any query string in the original url
    RewriteRule ^(.*)$ <?php print $this->platform->server->web_disable_url . '/' . $this->uri ?>?

</VirtualHost>
