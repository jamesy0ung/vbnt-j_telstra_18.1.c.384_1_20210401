package org.apache.felix.ipojo.test.scenarios.component;

import org.apache.felix.ipojo.annotations.Bind;
import org.apache.felix.ipojo.annotations.Component;
import org.apache.felix.ipojo.annotations.Requires;
import org.apache.felix.ipojo.annotations.Unbind;
import org.apache.felix.ipojo.test.scenarios.annotations.service.FooService;

@Component
public class FromDependency {

    @Requires(from="X")
    public FooService fs;
    
    @Unbind
    public void unbindBar() {
        
    }
    
    @Bind
    public void bindBar() {
        
    }
    
    @Unbind(from="both")
    public void unbindBaz() {
        
    }
    
    @Bind(from="both")
    public void bindBaz() {
        
    }
   
    
    @Requires
    public FooService fs2;
    
    @Bind(id="fs2", from="X")
    public void bindFS2() {
        
    }
    
    @Unbind(id="fs2")
    public void unbindFS2() {
        
    }
    
    @Requires(id="inv")
    public FooService fs2inv;
    
    @Bind(id="inv")
    public void bindFS2Inv() {
        
    }
    
    @Unbind(id="inv", from="X")
    public void unbindFS2Inv() {
        
    }
    
    
    
}
