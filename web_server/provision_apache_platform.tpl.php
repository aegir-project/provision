<Directory <?php print $publish_path; ?>>
    Order allow,deny
    Allow from all
<?php print $extra_config; ?>
</Directory>

