<?php

namespace Aegir\Provision\Common;

use Aegir\Provision\Provision;

trait ProvisionAwareTrait
{
    /**
     * @var Provision
     */
    protected $provision = NULL;
    
    /**
     * @param Provision $provision
     *
     * @return $this
     */
    public function setProvision(Provision $provision = NULL)
    {
        $this->provision = $provision;
        
        return $this;
    }
    
    /**
     * @return Provision
     */
    public function getProvision()
    {
    
        if (is_null($this->provision)) {
            return Provision::getProvision();
        }
    
        return $this->provision;
    }
}
