<?php

namespace Aegir\Provision\Common;

use Aegir\Provision\Provision;

trait ProvisionAwareTrait
{
    /**
     * @var Provision
     */
    protected $provision;
    
    /**
     * @param Provision $provision
     *
     * @return $this
     */
    public function setProvision(Provision $provision)
    {
        $this->provision = $provision;
        
        return $this;
    }
    
    /**
     * @return Provision
     */
    public function getProvision()
    {
        return $this->provision;
    }
}
