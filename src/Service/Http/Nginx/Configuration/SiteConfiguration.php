<?php
/**
 * @file SiteConfiguration.php
 *
 * NGINX Configuration for Site Context.
 */

namespace Aegir\Provision\Service\Http\Nginx\Configuration;

use Aegir\Provision\Configuration;

class SiteConfiguration extends Configuration {

    public $template = 'templates/vhost.tpl.php';

    // The template file to use when the site has been disabled.
    public $disabled_template = 'templates/vhost_disabled.tpl.php';

    public $description = 'virtual host configuration file';


    function filename() {
        $file = $this->context->getProperty('uri') . '.conf';
        return $this->context->getProvision()
                ->getConfig()
                ->get('config_path') . '/' . $this->service->provider->name . '/' . $this->service->getType() . '/vhost.d/' . $file;
    }

    function process() {
        parent::process();
        $this->data['http_port'] = $this->context->platform->getSubscription('http')->service->getProperty('http_port');
        $this->data['document_root'] = $this->context->platform->getProperty('document_root');
        $this->data['uri'] = $this->context->getProperty('uri');

        $this->data['site_path'] = $this->data['document_root'] . '/sites/' . $this->data['uri'];

        $this->data['db_type'] = $this->context->getSubscription('db')->service->getType();

        $this->data['db_name'] = $this->context->getSubscription('db')
            ->getProperty('db_name');
        $this->data['db_user'] = $this->context->getSubscription('db')
            ->getProperty('db_user');
        $this->data['db_passwd'] = $this->context->getSubscription('db')
            ->getProperty('db_password');
        $this->data['db_host'] = $this->context->getSubscription('db')->service->provider->getProperty('remote_host');

        $this->data['db_port'] = $this->context->getSubscription('db')->service->getCreds()['port'];

        $this->data['extra_config'] = '';

        $this->data['redirection'] = $this->context->getProperty('redirection');
        $this->data['ssl_redirection'] = $this->context->getProperty('ssl_redirection');
        $this->data['aliases'] = $this->context->getProperty('aliases');

        $this->data['script_user'] = $this->context->platform->getService('http')->provider->getProperty('script_user');
        $this->data['aegir_root'] = $this->service->provider->getProperty('aegir_root');
        $this->data['root'] = $this->context->platform->getProperty('root');
        $this->data['server_include_path'] = $this->context->platform->getProperty('root');

        if (empty($this->data['db_port'])) {
            $db_port = $this->context->getService('db')->getProperty('db_port ')? $this->context->getService('db')->getProperty('db_port ') : '3306';
        }
    }
}