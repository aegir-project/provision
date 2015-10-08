<?php

/**
 * @file
 * Define subject and body text for an email to the admin user created
 * for a site.
 *
 * At the very least you will want to send them the one-time login url, !login_url.
 */

$mail = array();
$mail['subject'] = dt('Your new site !site has been created');
$mail['body'] = dt("!username,

Your new site !site has been created.

You can now log in by clicking on this link or copying it into your browser:

!login_url

This is a one-time login, so it can be used only once.

Once you have logged in, you will be redirected to:

!edit_uri

Please remember to set your own admin password there.");

?>
