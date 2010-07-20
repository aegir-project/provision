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
      '--client_email' => 'site: email for the site owner',
      '--profile' => 'site: Drupal profile to use; default default',
    );
  }

  function init() {
    $this->parent_key = 'platform';

    parent::init();
    $this->type = 'site';
    
    $this->setProperty('platform');
    $this->is_oid('platform');

    // we need to set the alias root to the platform root, otherwise drush will cause problems.
    $this->root = $this->platform->root;
    $this->setProperty('uri');

    // set this because this path is accessed a lot in the code, especially in config files.
    $this->site_path = $this->root . '/sites/' . $this->uri;

    $this->setProperty('site_enabled', true);
    $this->setProperty('language', 'en');
    $this->setProperty('client_email');
    $this->setProperty('aliases', array(), TRUE);
    $this->setProperty('redirection', FALSE);

    // TODO :: figure out how to put this into service classes.
    $this->setProperty('ssl_enabled', 0);
    $this->setProperty('ssl_key', null);

    // this can potentially be handled by a Drupal sub class
    $this->setProperty('profile', 'default');
  }
}

