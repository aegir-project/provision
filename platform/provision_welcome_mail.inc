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
$mail['body'] = dt("!username,\n\nYour new site !site has been created.\n\nYou can now login to !site by clicking on this link or copying it into your browser:\n\n!login_url\n\nThis is a one-time login, so it can be used only once.\n\nOnce you have logged in, you will be redirected to !edit_uri so you can change your password.");

?>
