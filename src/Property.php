<?php
namespace Aegir\Provision;


/**
 * Class Property
 *
 * Use this to create dynamic properties for contexts.
 *
 * For example:
 *
 * <?php
 * static function option_documentation()
 * {
 *      return [
 *          'remote_host' =>
 *              Provision::newProperty()
 *                  ->description('server: host name')
 *                  ->required(TRUE)
 *                  ->default('localhost'),
 *                  ->validate(function($remote_host) {
 *                      // If remote_host doesn't resolve to anything, warn the user.
 *                     $ip = gethostbynamel($remote_host);
 *                     if (empty($ip)) {
 *                         throw new \RuntimeException("Hostname $remote_host does not resolve to an IP address. Please try again.");
 *                     }
 *                     return $remote_host;
 *                  }),
 *      ];
 * }
 * ?>
 *
 * @package Aegir\Provision
 */
class Property {
    
    public $description = '';
    public $default = NULL;
    public $required = FALSE;
    public $validate;
    
    /**
     * Allow "backwards" compatibility: return the description when converting to a string.
     * @return string
     */
    function __toString()
    {
        return $this->description;
    }
    
    /**
     * Property constructor.
     *
     * Set description and default validate callable.
     *
     * @param null $description
     */
    public function __construct($description = NULL) {
        $this->description($description);
        $this->validate(function ($answer) {
            if ($this->required && empty($answer)) {
                throw new \RuntimeException('Property is required.');
            }
            else {
                return $answer;
            }
        });
        return $this;
    }
    
    /**
     * Set the description of this property.
     *
     * @param string $description
     *
     * @return $this
     */
    public function description($description) {
        $this->description = $description;
        return $this;
    }
    
    /**
     * Set the default value of this property.
     *
     * If value is callable, will set the return value of the callable as the default value.
     *
     * @param string|callable $default
     *
     * @return $this
     */
    public function default($default) {
        if (is_callable($default)) {
            $this->default = $default();
        }
        else {
            $this->default = $default;
        }
        return $this;
    }
    
    /**
     * Set if this Property is required or not.
     *
     * @param bool $required
     *
     * @return $this
     */
    public function required($required = TRUE) {
        $this->required = $required;
        return $this;
    }
    
    /**
     * Set the validation function for this property.
     *
     * @param callable $callable
     *
     * @return $this
     */
    public function validate($callable) {
        $this->validate = $callable;
        return $this;
    }
}