<VirtualHost *:<?php print $this->site_port; ?>>
    <?php if ($this->site_mail) : ?>
      ServerAdmin <?php  print $this->site_mail; ?> 
    <?php endif;?>

    <?php if (is_array($this->aliases) && count($this->aliases)): ?>
      ServerName <?php print array_pop($this->aliases); ?>
    
      <?php if (count($this->aliases)): ?>
        ServerAlias <?php print join(" ", $this->aliases); ?>
      <?php endif; ?>
    <?php else:
    # this should never happen and has the potential of creating an infinite redirection loop
     ?>
      ServerName <?php print $this->uri ?>
    <?php endif; ?>

<?php if ($ssl_redirect): ?>
    RedirectMatch permanent ^(.*) https://<?php print $this->uri ?>$1
<?php else: ?>
    RedirectMatch permanent ^(.*) http://<?php print $this->uri ?>$1
<?php endif; ?>
</VirtualHost>
