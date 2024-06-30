/* 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
package org.apache.felix.ipojo.test.scenarios.service.dependency;

import java.util.Properties;

import org.apache.felix.ipojo.ComponentInstance;
import org.apache.felix.ipojo.architecture.Architecture;
import org.apache.felix.ipojo.architecture.InstanceDescription;
import org.apache.felix.ipojo.junit4osgi.OSGiTestCase;
import org.apache.felix.ipojo.test.scenarios.service.dependency.service.CheckService;
import org.apache.felix.ipojo.test.scenarios.util.Utils;
import org.osgi.framework.ServiceReference;

public class ProxiedDelayedOptionalMultipleDependencies extends OSGiTestCase {

    ComponentInstance instance1, instance2, instance3, instance4;
    ComponentInstance fooProvider1, fooProvider2;
    
    public void setUp() {		
        try {
            Properties i1 = new Properties();
            i1.put("instance.name","Simple");
            instance1 = Utils.getFactoryByName(getContext(), "ProxiedSimpleOptionalMultipleCheckServiceProvider").createComponentInstance(i1);
            instance1.stop();
        
            Properties i2 = new Properties();
            i2.put("instance.name","Void");
            instance2 = Utils.getFactoryByName(getContext(), "ProxiedVoidOptionalMultipleCheckServiceProvider").createComponentInstance(i2);
            instance2.stop();
        
            Properties i3 = new Properties();
            i3.put("instance.name","Object");
            instance3 = Utils.getFactoryByName(getContext(), "ProxiedObjectOptionalMultipleCheckServiceProvider").createComponentInstance(i3);
            instance3.stop();
        
            Properties i4 = new Properties();
            i4.put("instance.name","Ref");
            instance4 = Utils.getFactoryByName(getContext(), "ProxiedRefOptionalMultipleCheckServiceProvider").createComponentInstance(i4);
            instance4.stop();
        
            Properties prov = new Properties();
            prov.put("instance.name","FooProvider1");
            fooProvider1 = Utils.getFactoryByName(getContext(), "FooProviderType-1").createComponentInstance(prov);
        
            Properties prov2 = new Properties();
            prov2.put("instance.name","FooProvider2");
            fooProvider2 = Utils.getFactoryByName(getContext(), "FooProviderType-1").createComponentInstance(prov2);
        } catch(Exception e) { fail(e.getMessage()); }		
    }
    
    public void tearDown() {
        instance1.dispose();
        instance2.dispose();
        instance3.dispose();
        instance4.dispose();
        fooProvider1.dispose();
        fooProvider2.dispose();
        instance1 = null;
        instance2 = null;
        instance3 = null;
        instance4 = null;
        fooProvider1 = null;
        fooProvider2 = null;
    }
    
    public void testSimple() {
        instance1.start();
        ServiceReference arch_ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), instance1.getInstanceName());
        assertNotNull("Check architecture availability", arch_ref);
        InstanceDescription id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 1", id.getState() == ComponentInstance.VALID);
        
        ServiceReference cs_ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), instance1.getInstanceName());
        assertNotNull("Check CheckService availability", cs_ref);
        CheckService cs = (CheckService) getContext().getService(cs_ref);
        Properties props = cs.getProps();
        //Check properties
        assertTrue("check CheckService invocation - 0", ((Boolean)props.get("result")).booleanValue());
        assertEquals("check void bind invocation - 0", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 0", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 0", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 0", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 0", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 0", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 0", ((Integer)props.get("int")).intValue(), 2);
        assertEquals("Check FS invocation (long) - 0", ((Long)props.get("long")).longValue(), 2);
        assertEquals("Check FS invocation (double) - 0", ((Double)props.get("double")).doubleValue(), 2.0);
        
        fooProvider1.stop();
        
        id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 2", id.getState() == ComponentInstance.VALID);
        
        cs = (CheckService) getContext().getService(cs_ref);
        props = cs.getProps();
        //Check properties
        assertTrue("check CheckService invocation - 3", ((Boolean)props.get("result")).booleanValue()); // True, it still one provider.
        assertEquals("check void bind invocation - 3", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 3", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 3", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 3", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 3", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 3", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 3", ((Integer)props.get("int")).intValue(), 1);
        assertEquals("Check FS invocation (long) - 3", ((Long)props.get("long")).longValue(), 1);
        assertEquals("Check FS invocation (double) - 3", ((Double)props.get("double")).doubleValue(), 1.0);
        
        fooProvider2.stop();
        
        id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 5", id.getState() == ComponentInstance.VALID);
        
        cs = (CheckService) getContext().getService(cs_ref);
        props = cs.getProps();
        //Check properties
        assertFalse("check CheckService invocation - 4", ((Boolean)props.get("result")).booleanValue()); // False, no more provider.
        assertEquals("check void bind invocation - 4", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 4", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 4", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 4", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 4", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 4", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 4", ((Integer)props.get("int")).intValue(), 0);
        assertEquals("Check FS invocation (long) - 4", ((Long)props.get("long")).longValue(), 0);
        assertEquals("Check FS invocation (double) - 4", ((Double)props.get("double")).doubleValue(), 0.0);
        
        id = null;
        cs = null;
        getContext().ungetService(arch_ref);
        getContext().ungetService(cs_ref);		
        instance1.stop();
    }
    
    public void testVoid() {
        instance2.start();
        ServiceReference arch_ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), instance2.getInstanceName());
        assertNotNull("Check architecture availability", arch_ref);
        InstanceDescription id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance invalidity - 1", id.getState() == ComponentInstance.VALID);
        
        ServiceReference cs_ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), instance2.getInstanceName());
        assertNotNull("Check CheckService availability", cs_ref);
        CheckService cs = (CheckService) getContext().getService(cs_ref);
        Properties props = cs.getProps();
        //Check properties
        assertTrue("check CheckService invocation - 0", ((Boolean)props.get("result")).booleanValue());
        assertEquals("check void bind invocation - 0", ((Integer)props.get("voidB")).intValue(), 2);
        assertEquals("check void unbind callback invocation - 0", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 0", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 0", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 0", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 0", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 0", ((Integer)props.get("int")).intValue(), 2);
        assertEquals("Check FS invocation (long) - 0", ((Long)props.get("long")).longValue(), 2);
        assertEquals("Check FS invocation (double) - 0", ((Double)props.get("double")).doubleValue(), 2.0);
        
        fooProvider1.stop();
        
        id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 2", id.getState() == ComponentInstance.VALID);
        
        cs = (CheckService) getContext().getService(cs_ref);
        props = cs.getProps();
        //Check properties
        assertTrue("check CheckService invocation - 3", ((Boolean)props.get("result")).booleanValue()); // True, two providers are here
        assertEquals("check void bind invocation - 3", ((Integer)props.get("voidB")).intValue(), 2);
        assertEquals("check void unbind callback invocation - 3", ((Integer)props.get("voidU")).intValue(), 1);
        assertEquals("check object bind callback invocation - 3", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 3", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 3", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 3", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 3", ((Integer)props.get("int")).intValue(), 1);
        assertEquals("Check FS invocation (long) - 3", ((Long)props.get("long")).longValue(), 1);
        assertEquals("Check FS invocation (double) - 3", ((Double)props.get("double")).doubleValue(), 1.0);
        
        fooProvider2.stop();
        
        id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 3", id.getState() == ComponentInstance.VALID);
        
        cs = (CheckService) getContext().getService(cs_ref);
        props = cs.getProps();
        //Check properties
        assertFalse("check CheckService invocation - 4", ((Boolean)props.get("result")).booleanValue()); // False : no provider
        assertEquals("check void bind invocation - 4", ((Integer)props.get("voidB")).intValue(), 2);
        assertEquals("check void unbind callback invocation - 4", ((Integer)props.get("voidU")).intValue(), 2);
        assertEquals("check object bind callback invocation - 4", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 4", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 4", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 4", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 4", ((Integer)props.get("int")).intValue(), 0);
        assertEquals("Check FS invocation (long) - 4", ((Long)props.get("long")).longValue(), 0);
        assertEquals("Check FS invocation (double) - 4", ((Double)props.get("double")).doubleValue(), 0.0);
        
        id = null;
        cs = null;
        getContext().ungetService(arch_ref);
        getContext().ungetService(cs_ref);
        instance2.stop();
    }
    
    public void testObject() {
        instance3.start();
        ServiceReference arch_ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), instance3.getInstanceName());
        assertNotNull("Check architecture availability", arch_ref);
        InstanceDescription id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance invalidity - 1", id.getState() == ComponentInstance.VALID);
        
        ServiceReference cs_ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), instance3.getInstanceName());
        assertNotNull("Check CheckService availability", cs_ref);
        CheckService cs = (CheckService) getContext().getService(cs_ref);
        Properties props = cs.getProps();
        //Check properties
        assertTrue("check CheckService invocation - 0", ((Boolean)props.get("result")).booleanValue());
        assertEquals("check void bind invocation - 0", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 0", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 0", ((Integer)props.get("objectB")).intValue(), 2);
        assertEquals("check object unbind callback invocation - 0", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 0", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 0", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 0", ((Integer)props.get("int")).intValue(), 2);
        assertEquals("Check FS invocation (long) - 0", ((Long)props.get("long")).longValue(), 2);
        assertEquals("Check FS invocation (double) - 0", ((Double)props.get("double")).doubleValue(), 2.0);
        
        fooProvider1.stop();
        
        id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 2", id.getState() == ComponentInstance.VALID);
        
        cs = (CheckService) getContext().getService(cs_ref);
        props = cs.getProps();
        //Check properties
        assertTrue("check CheckService invocation - 3", ((Boolean)props.get("result")).booleanValue());
        assertEquals("check void bind invocation - 3", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 3", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 3", ((Integer)props.get("objectB")).intValue(), 2);
        assertEquals("check object unbind callback invocation - 3", ((Integer)props.get("objectU")).intValue(), 1);
        assertEquals("check ref bind callback invocation - 3", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 3", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 3", ((Integer)props.get("int")).intValue(), 1);
        assertEquals("Check FS invocation (long) - 3", ((Long)props.get("long")).longValue(), 1);
        assertEquals("Check FS invocation (double) - 3", ((Double)props.get("double")).doubleValue(), 1.0);
        
        fooProvider2.stop();
        
        id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 5", id.getState() == ComponentInstance.VALID);
        
        cs = (CheckService) getContext().getService(cs_ref);
        props = cs.getProps();
        //Check properties
        assertFalse("check CheckService invocation - 0", ((Boolean)props.get("result")).booleanValue()); // False : no provider
        assertEquals("check void bind invocation - 0", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 0", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 0", ((Integer)props.get("objectB")).intValue(), 2);
        assertEquals("check object unbind callback invocation - 0", ((Integer)props.get("objectU")).intValue(), 2);
        assertEquals("check ref bind callback invocation - 0", ((Integer)props.get("refB")).intValue(), 0);
        assertEquals("check ref unbind callback invocation - 0", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 0", ((Integer)props.get("int")).intValue(), 0);
        assertEquals("Check FS invocation (long) - 0", ((Long)props.get("long")).longValue(), 0);
        assertEquals("Check FS invocation (double) - 0", ((Double)props.get("double")).doubleValue(), 0.0);
        
        id = null;
        cs = null;
        getContext().ungetService(arch_ref);
        getContext().ungetService(cs_ref);		
        instance3.stop();
    }
    
    public void testRef() {
        instance4.start();
        ServiceReference arch_ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), instance4.getInstanceName());
        assertNotNull("Check architecture availability", arch_ref);
        InstanceDescription id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance invalidity - 1", id.getState() == ComponentInstance.VALID);
        
        ServiceReference cs_ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), instance4.getInstanceName());
        assertNotNull("Check CheckService availability", cs_ref);
        CheckService cs = (CheckService) getContext().getService(cs_ref);
        Properties props = cs.getProps();
        //Check properties
        assertTrue("check CheckService invocation - 0", ((Boolean)props.get("result")).booleanValue());
        assertEquals("check void bind invocation - 0", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 0", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 0", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 0", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 0", ((Integer)props.get("refB")).intValue(), 2);
        assertEquals("check ref unbind callback invocation - 0", ((Integer)props.get("refU")).intValue(), 0);
        assertEquals("Check FS invocation (int) - 0", ((Integer)props.get("int")).intValue(), 2);
        assertEquals("Check FS invocation (long) - 0", ((Long)props.get("long")).longValue(), 2);
        assertEquals("Check FS invocation (double) - 0", ((Double)props.get("double")).doubleValue(), 2.0);
        
        fooProvider1.stop();
        
        id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 2", id.getState() == ComponentInstance.VALID);
        
        cs = (CheckService) getContext().getService(cs_ref);
        props = cs.getProps();
        //Check properties
        assertTrue("check CheckService invocation - 3", ((Boolean)props.get("result")).booleanValue());
        assertEquals("check void bind invocation - 3", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 3", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 3", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 3", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 3", ((Integer)props.get("refB")).intValue(), 2);
        assertEquals("check ref unbind callback invocation - 3", ((Integer)props.get("refU")).intValue(), 1);
        assertEquals("Check FS invocation (int) - 3", ((Integer)props.get("int")).intValue(), 1);
        assertEquals("Check FS invocation (long) - 3", ((Long)props.get("long")).longValue(), 1);
        assertEquals("Check FS invocation (double) - 3", ((Double)props.get("double")).doubleValue(), 1.0);
        
        fooProvider2.stop();
        
        id = ((Architecture) getContext().getService(arch_ref)).getInstanceDescription();
        assertTrue("Check instance validity - 3", id.getState() == ComponentInstance.VALID);
        
        cs = (CheckService) getContext().getService(cs_ref);
        props = cs.getProps();
        //Check properties
        assertFalse("check CheckService invocation - 0", ((Boolean)props.get("result")).booleanValue()); // False : no provider
        assertEquals("check void bind invocation - 0", ((Integer)props.get("voidB")).intValue(), 0);
        assertEquals("check void unbind callback invocation - 0", ((Integer)props.get("voidU")).intValue(), 0);
        assertEquals("check object bind callback invocation - 0", ((Integer)props.get("objectB")).intValue(), 0);
        assertEquals("check object unbind callback invocation - 0", ((Integer)props.get("objectU")).intValue(), 0);
        assertEquals("check ref bind callback invocation - 0", ((Integer)props.get("refB")).intValue(), 2);
        assertEquals("check ref unbind callback invocation - 0", ((Integer)props.get("refU")).intValue(), 2);
        assertEquals("Check FS invocation (int) - 0", ((Integer)props.get("int")).intValue(), 0);
        assertEquals("Check FS invocation (long) - 0", ((Long)props.get("long")).longValue(), 0);
        assertEquals("Check FS invocation (double) - 0", ((Double)props.get("double")).doubleValue(), 0.0);
        
        id = null;
        cs = null;
        getContext().ungetService(arch_ref);
        getContext().ungetService(cs_ref);
        instance4.stop();
    }

    
}
