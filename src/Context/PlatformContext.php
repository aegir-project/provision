<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Application;
use Aegir\Provision\Console\Config;
use Aegir\Provision\ServiceSubscriber;
use Aegir\Provision\Provision;
use Symfony\Component\Config\Definition\ConfigurationInterface;

/**
 * Class PlatformContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_platform
 */
class PlatformContext extends ServiceSubscriber implements ConfigurationInterface
{
    /**
     * @var string
     */
    public $type = 'platform';
    const TYPE = 'platform';

    /**
     * @var \Aegir\Provision\Context\ServerContext;
     */
    public $web_server;

    /**
     * PlatformContext constructor.
     *
     * @param $name
     * @param Provision $provision
     * @param array $options
     */
    function __construct(
        $name,
        Provision $provision = NULL,
        $options = []
    ) {
        parent::__construct($name, $provision, $options);

        // Load "web_server" context.
        // There is no need to validate for $this->properties['web_server'] because the config system does that.
//        $this->web_server = $application->getContext($this->properties['web_server']);

        // Set document_root_full property with the full path to docroot.
        if (empty($this->getProperty('document_root'))) {
            $this->setProperty('document_root_full', $this->getProperty('root'));
        }
        else {
            $this->setProperty('document_root_full', $this->getProperty('root') . DIRECTORY_SEPARATOR . $this->getProperty('document_root'));
        }
        $this->getProperties();
    }
    
    static function option_documentation()
    {
        $default_root = getcwd();
        if ($default_root == Config::getHomeDir() || !is_writable($default_root)) {
            $default_root = null;
        }

        $options = [
            'root' =>
                Provision::newProperty()
                    ->description('Path to source code. Enter an absolute path or relative to ' . getcwd() . ". If path does not exist, it will be created.")
                    ->defaultValue($default_root)
                    ->required(TRUE)
                    ->forceAsk(TRUE)
                    ->validate(function($path) {
                        if (!Provision::fs()->isAbsolutePath($path)) {

                            // realpath() doesn't work if file is missing.
                            // Our root might not exist until verify.
                            // Using realpath() helps us verify relative path with "." or ".." in it.
                            if (file_exists($path)) {
                                $path = realpath($path);
                            }
                            // elseif !empty is to prevent appending DIRECTORY_SEPARATOR when there is not path.
                            elseif (!empty($path)) {

                                // Attempt to use realpath on the first part of the path
                                $path_parts = explode(DIRECTORY_SEPARATOR, $path);
                                $path_last_dir = array_pop($path_parts);
                                $path = getcwd() . DIRECTORY_SEPARATOR . $path;

                                $parent_path = realpath(rtrim($path, $path_last_dir));

                                if (!empty($parent_path)) {
                                    $path = $parent_path . DIRECTORY_SEPARATOR . $path_last_dir;
                                }
                            }
                        }
                        if (empty($path)){
                            throw new \Exception('Path cannot be empty.');
                        }
                        if ($path == Config::getHomeDir()) {
                            throw new \Exception("You can't use your home directory as code root. Please try again.");
                        }
                        if (file_exists($path) && !is_writable($path)) {
                            throw new \Exception("That path is not writable using the current user. Please try again.");
                        }
                        Provision::getProvision()->io()->successLite('Using root ' . $path);

                        return $path;
                    })
                ,
            'git_url' =>
                Provision::newProperty()
                    ->description('platform: Git repository remote URL.')
                    ->required(FALSE)
                    ->validate(function($git_url) {
                        if (empty(trim($git_url))) {
                            return;
                        }
                        Provision::getProvision()->io()->comment('Checking git remote...');

                        // Use git ls-remote to detect a valid and accessible git URL.
                        $result = Provision::getProvision()->getTasks()->taskExec('git ls-remote')
                            ->arg($git_url)
                            ->silent(!Provision::getProvision()->getOutput()->isVerbose())
                            ->run();

                        if (!$result->wasSuccessful()) {
                            throw new \RuntimeException("Unable to connect to git remote $git_url. Please check access and try again.");
                        }

                        Provision::getProvision()->io()->successLite('Connected to git remote.');

                        // @TODO: Parse brances and tags.

                        return $git_url;
                    })
            ,
            'makefile' =>
                Provision::newProperty()
                    ->description('platform: Drush makefile to use for building the platform. May be a path or URL.')
                    ->required(FALSE)
                    ->validate(function($makefile) {
                        if (empty($makefile)) {
                            return $makefile;
                        }
                        $parsed = parse_url($makefile);

                        // If parsed is empty, it couldn't be read as a URL or filename.
                        if (empty($parsed)) {
                            throw new \RuntimeException("The makefile at {$makefile} could not be read.");
                        }
                        // If array is only path, it is a file path.
                        elseif (count(array_keys($parsed)) == 1 && isset($parsed['path'])) {
                            if (is_readable($parsed['path'])) {
                                return $makefile;
                            }
                            else {
                                throw new \RuntimeException("The makefile at {$makefile} could not be read.");
                            }
                        }
                        // Otherwise, makefile is a URL. Check if we can access it.
                        else {
                            try {
                                $content = @file_get_contents($makefile);
                                if ($content === false) {
                                    throw new \RuntimeException("The makefile at {$makefile} could not be read.");
                                } else {
                                    return $makefile;
                                }
                            }
                            catch (\Exception $e) {
                                throw new \RuntimeException("The makefile at {$makefile} could not be read.");
                            }
                        }
                        
                        return $makefile;
                    })
            
            ,
            'make_working_copy' =>
                Provision::newProperty()
                    ->description('platform: Specifiy TRUE to build the platform with the Drush make --working-copy option.')
                    ->required(FALSE)
            ,
            'document_root' =>
                Provision::newProperty()
                    ->description('platform: Relative path to the "document root" in your source code. Leave blank if docroot is the root.')
                    ->required(FALSE)
            ,
        ];

        return $options;
    }

    /**
     * Platforms require a web (http) server.
     *
     * @return array
     */
    public static function serviceRequirements() {
        return ['http'];
    }
    
    /**
     * Output extra info before verifying.
     */
    public function verify()
    {
        $this->getProvision()->io()->customLite($this->getProperty('root'), 'Code Root: ', 'info');
        $this->getProvision()->io()->customLite($this->getProperty('document_root'), 'Document Root: ', 'info');

        if ($this->getProperty('makefile')) {
            $this->getProvision()->io()->customLite($this->getProperty('makefile'), 'Makefile: ', 'info');
        }

        $this->getProvision()->io()->customLite($this->config_path, 'Configuration File: ', 'info');

        $this->getProvision()->io()->newLine();
    
        $tasks = [];

        // If platform files don't exist, but has git url or makefile, build now.
        if ($this->getProperty('git_url')) {

            if ($this->fs->exists($this->getProperty('root'))) {
                $tasks['platform.files'] = $this->getProvision()->newTask()
                    ->success('Cloning git repository... Files already exist.');
            }
            else {
                $tasks['platform.git'] = $this->getProvision()->newTask()
                    ->start('Cloning git repository...')
                    ->execute(function () {
                        if (!$this->fs->exists($this->getProperty('root'))) {
                            $this->getProvision()->io()->warningLite('Root path does not exist. Cloning source code from git repository ' . $this->getProperty('git_url') . ' to ' . $this->getProperty('root'));

                            return $this->getProvision()->getTasks()->taskExec("git clone")
                                ->arg($this->getProperty('git_url'))
                                ->arg($this->getProperty('root'))
                                ->silent(!$this->getProvision()->getOutput()->isVerbose())
                                ->run()
                                ->getExitCode()
                                ;
                        }
                        else {
                            // @TODO: Check git remote URl to ensure it matches.
                            return 0;
                        }
                    });
            }

        }
        // @TODO: I did get makefiles in git to work hosting_git-7.x-3.x. We can do it here as well.
        elseif ($this->getProperty('makefile')) {

            if ($this->fs->exists($this->getProperty('root'))) {
                $tasks['platform.files'] = $this->getProvision()->newTask()
                    ->start('Building platform from makefile... Files already exist.');
            }
            else {
                $tasks['platform.make'] = $this->getProvision()->newTask()
                    ->start('Building platform from makefile...')
                    ->execute(function () {
                        $drush = realpath(__DIR__ . '/../../bin/drush');
                        $command = $this->getProvision()->getTasks()->taskExec($drush)
                                ->arg('make')
                                ->arg($this->getProperty('makefile'))
                                ->arg($this->getProperty('root'))
                                ->silent(!$this->getProvision()->getOutput()->isVerbose())
                                ->getCommand();

                        if ($this->getProperty('make_working_copy')) {
                            $command .= ' --working-copy --no-gitinfofile --no-gitinfofile --no-gitprojectinfo';
                        }

                        return $this->getService('http')->provider->shell_exec($command, NULL, 'exit');
                    });

            }
        }

        // If files already exist, say so.
        $tasks['platform.found'] = $this->getProvision()->newTask()
            ->start('Checking root path for files...')
            ->execute(function () {
                return $this->fs->exists($this->getProperty('root'))? 0: 1;
            });

        return $tasks;
        
//        return parent::verify();
    }

    /**
     * Overrides Context::save() to remove the document_root_full property.
     *
     * @TODO: Figure out a better way to avoid storing system generated properties.
     */
    public function save(){
        unset($this->properties['document_root_full']);
        return parent::save();
    }
}
