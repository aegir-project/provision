<?php
// $Id$

/**
 * @file Provision named context site class.
 */

class provisionContext_site extends provisionContext {
  public $parent_key = 'platform';

  static function option_documentation() {
    return array(
      '--platform' => 'site: the platform the site is run on',
      '--db_server' => 'site: the db server the site is run on',
      '--uri' => 'site: example.com URI, no http:// or trailing /',
      '--language' => 'site: site language; default en',
      '--aliases' => 'site: comma-separated URIs',
      '--redirection' => 'site: boolean for whether --aliases should redirect; default false',
      '--client_name' => 'site: machine name of the client that owns this site',
      '--profile' => 'site: Drupal profile to use; default default',
    );
  }

  function init_site() {
    $this->setProperty('uri');

     // we need to set the alias root to the platform root, otherwise drush will cause problems.
    $this->root = $this->platform->root;

    // set this because this path is accessed a lot in the code, especially in config files.
    $this->site_path = $this->root . '/sites/' . $this->uri;

    $this->setProperty('site_enabled', true);
    $this->setProperty('language', 'en');
    $this->setProperty('client_name');
    $this->setProperty('aliases', array(), TRUE);
    $this->setProperty('redirection', FALSE);
    $this->setProperty('cron_key', '');

    // this can potentially be handled by a Drupal sub class
    $this->setProperty('profile', 'default');
  }
}

