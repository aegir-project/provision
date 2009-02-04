<?php
// $Id$
/**
 *  @file
 *    Rebuild all the caches
 */
cache_clear_all();
provision_log('notice', t('Cleared all caches'));

node_types_rebuild();
provision_log('notice', t('Rebuild node type cache'));

module_rebuild_cache();
provision_log('notice', t('Rebuild module cache'));

system_theme_data();
provision_log('notice', t('Rebuild theme cache'));

node_access_rebuild();
provision_log('notice', t('Rebuild node access cache'));

menu_rebuild();
provision_log('notice', t('Rebuild menu cache'));
