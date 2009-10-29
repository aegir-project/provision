<Directory <?php print $publish_path; ?>>

    Options Indexes FollowSymLinks MultiViews
    AllowOverride All
    Order allow,deny
    Allow from all
<?php print $extra_config; ?>
</Directory>

