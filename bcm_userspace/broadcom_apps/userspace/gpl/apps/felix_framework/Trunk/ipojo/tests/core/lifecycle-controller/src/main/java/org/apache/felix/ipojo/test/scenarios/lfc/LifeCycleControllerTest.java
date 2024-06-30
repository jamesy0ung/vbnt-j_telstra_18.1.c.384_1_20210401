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
package org.apache.felix.ipojo.test.scenarios.lfc;

import java.util.Properties;

import org.apache.felix.ipojo.ComponentInstance;
import org.apache.felix.ipojo.Factory;
import org.apache.felix.ipojo.junit4osgi.OSGiTestCase;
import org.apache.felix.ipojo.test.scenarios.lfc.service.CheckService;
import org.apache.felix.ipojo.test.scenarios.util.Utils;
import org.osgi.framework.ServiceReference;

/**
 * @author <a href="mailto:dev@felix.apache.org">Felix Project Team</a>
 */
public class LifeCycleControllerTest extends OSGiTestCase {

    private ComponentInstance under;

    private Factory factory;

    public void setUp() {
        factory = Utils.getFactoryByName(getContext(), "LFC-Test");
    }

    public void testOne() {
        Properties props = new Properties();
        props.put("conf", "foo");
        props.put("instance.name","under1");
        under = Utils.getComponentInstance(getContext(), "LFC-Test", props);

        // The conf is correct, the PS must be provided
        ServiceReference ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), "under1");
        assertNotNull("Check service availability -1", ref);
        CheckService cs = (CheckService) getContext().getService(ref);
        assertTrue("Check state 1", cs.check());
        getContext().ungetService(ref);
        cs = null;

        // Reconfigure the instance with a bad configuration
        props.put("conf", "bar"); // Bar is a bad conf
        try {
            factory.reconfigure(props);
        } catch (Exception e) {
            fail("The reconfiguration is not unacceptable and seems unacceptable : " + props);
        }

        // The instance should now be invalid 
        ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), "under1");
        assertNull("Check service availability -2", ref);

        // Reconfigure the instance with a valid configuration
        props.put("conf", "foo"); // Bar is a bad conf
        try {
            factory.reconfigure(props);
        } catch (Exception e) {
            fail("The reconfiguration is not unacceptable and seems unacceptable (2) : " + props);
        }

        ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), "under1");
        assertNotNull("Check service availability -3", ref);
        cs = (CheckService) getContext().getService(ref);
        assertTrue("Check state 2", cs.check());
        getContext().ungetService(ref);
        cs = null;

        under.dispose();
    }

    /**
     * This test must be removed as it is not compliant with OSGi. It unregisters a service during the creation of the 
     * service instance, so the returned object is null. 
     */
    public void notestTwo() {
        Properties props = new Properties();
        props.put("conf", "bar");
        props.put("instance.name","under2");
        under = Utils.getComponentInstance(getContext(), "LFC-Test", props);

        // The conf is incorrect, but the test can appears only when the object is created : the PS must be provided
        ServiceReference ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), "under2");
        assertNotNull("Check service availability -1", ref);

        System.out.println("CS received : " + getContext().getService(ref));
        CheckService cs = (CheckService) getContext().getService(ref);
        assertNotNull("Assert CS not null", cs);
        try {
            assertFalse("Check state (false)", cs.check());
        } catch (Throwable e) {
            e.printStackTrace();
            fail(e.getMessage());
        }

        // As soon as the instance is created, the service has to disappear :
        ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), "under2");
        assertNull("Check service availability -2", ref);

        // Reconfigure the instance with a correct configuration
        props.put("conf", "foo");
        try {
            factory.reconfigure(props);
        } catch (Exception e) {
            fail("The reconfiguration is not unacceptable and seems unacceptable : " + props);
        }

        ref = Utils.getServiceReferenceByName(getContext(), CheckService.class.getName(), "under2");
        assertNotNull("Check service availability -3", ref);
        cs = (CheckService) getContext().getService(ref);
        assertTrue("Check state ", cs.check());
        getContext().ungetService(ref);
        cs = null;

        under.dispose();
    }

}
