<VirtualHost *:80>
    <?php if ($site_mail) : ?>
      ServerAdmin <?php  print $site_mail; ?> 
    <?php endif;?>
    DocumentRoot <?php print $publish_path; ?> 
    
    ServerName <?php print $site_url; ?>

    RedirectMatch ^(.*)$ <?php print $redirect_url ?>

    <?php if (is_array($site_aliases)) :
     foreach ($site_aliases as $alias_url) : ?>
       ServerAlias <?php print $alias_url; ?>
     <?php
       endforeach;
     endif; ?>

</VirtualHost>
