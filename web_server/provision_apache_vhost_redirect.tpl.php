<VirtualHost *:<?php print $site_port; ?>>
    <?php if ($site_mail) : ?>
      ServerAdmin <?php  print $site_mail; ?> 
    <?php endif;?>

    <?php if (is_array($aliases) && count($aliases)): ?>
      ServerName <?php print array_pop($aliases); ?>
    
      <?php if (count($aliases)): ?>
        ServerAlias <?php print join(" ", $aliases); ?>
      <?php endif; ?>
    <?php else:
    # this should never happen and has the potential of creating an infinite redirection loop
     ?>
      ServerName <?php print $site_url ?>
    <?php endif; ?>

<?php if ($ssl_redirect): ?>
    RedirectMatch permanent ^(.*) https://<?php print $site_url ?>$1
<?php else: ?>
    RedirectMatch permanent ^(.*) http://<?php print $site_url ?>$1
<?php endif; ?>
</VirtualHost>
