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
package org.apache.felix.ipojo.test.composite.instantiator;

import java.util.Properties;

import org.apache.felix.ipojo.ComponentFactory;
import org.apache.felix.ipojo.ComponentInstance;
import org.apache.felix.ipojo.Factory;
import org.apache.felix.ipojo.ServiceContext;
import org.apache.felix.ipojo.architecture.Architecture;
import org.apache.felix.ipojo.architecture.InstanceDescription;
import org.apache.felix.ipojo.composite.CompositeInstanceDescription;
import org.apache.felix.ipojo.junit4osgi.OSGiTestCase;
import org.apache.felix.ipojo.test.composite.service.BarService;
import org.apache.felix.ipojo.test.composite.util.Utils;
import org.osgi.framework.ServiceReference;

public class SimpleInstantiation extends OSGiTestCase {

	private ComponentFactory bar1Factory;
	private ComponentInstance empty;
	
	public void setUp() {
		bar1Factory = (ComponentFactory) Utils.getFactoryByName(getContext(), "composite.bar.1");
		Factory fact = Utils.getFactoryByName(getContext(), "composite.empty");
		Properties props = new Properties();
		props.put("instance.name","empty-X");
		try {
			empty = fact.createComponentInstance(props);
		} catch(Exception e) {
			fail("Cannot create the empty composite : " + e.getMessage());
		}
	}
	
	public void tearDown() {
		empty.dispose();
		empty = null;
	}
	
	public void testCreation() {
		Properties props = new Properties();
		props.put("instance.name","under");
		ComponentInstance under = null;
		try {
			under = bar1Factory.createComponentInstance(props);
		} catch(Exception e) {
		    e.printStackTrace();
			fail("Cannot instantiate under : " + e.getMessage());
		}
		assertTrue("Check instance validity", under.getState() == ComponentInstance.VALID);
		under.dispose();
	}
	
	public void testServiceAvailability() {
		Properties props = new Properties();
		props.put("instance.name","under");
		ComponentInstance under = null;
		try {
			under = bar1Factory.createComponentInstance(props);
		} catch(Exception e) {
			fail("Cannot instantiate under : " + e.getMessage());
		}	
		assertTrue("Check instance validity", under.getState() == ComponentInstance.VALID);
		ServiceContext sc = Utils.getServiceContext(under);
		assertNotNull("Check service availability", sc.getServiceReference(BarService.class.getName()));
		
		under.dispose();
	}
	
	public void testCreationLevel2() {
		ServiceContext sc = Utils.getServiceContext(empty);
		Properties props = new Properties();
		props.put("instance.name","under");
		ComponentInstance under = null;
		try {
			under = bar1Factory.createComponentInstance(props, sc);
		} catch(Exception e) {
			fail("Cannot instantiate under : " + e.getMessage());
		}
		assertTrue("Check instance validity", under.getState() == ComponentInstance.VALID);
		under.dispose();
	}
	
	public void testServiceAvailabilityLevel2() {
		ServiceContext sc = Utils.getServiceContext(empty);
		Properties props = new Properties();
		props.put("instance.name","under-X");
		ComponentInstance under = null;
		try {
			under = bar1Factory.createComponentInstance(props, sc);
		} catch(Exception e) {
			fail("Cannot instantiate under : " + e.getMessage());
		}	
		assertTrue("Check instance validity", under.getState() == ComponentInstance.VALID);
		ServiceContext sc2 = Utils.getServiceContext(under);
		assertNotNull("Check service availability", sc2.getServiceReference(BarService.class.getName()));
		
		under.dispose();
	}
	
	public void testFactoryManagement() {
		Properties props = new Properties();
		props.put("instance.name","under");
		ComponentInstance under = null;
		try {
			under = bar1Factory.createComponentInstance(props);
		} catch(Exception e) {
			fail("Cannot instantiate under : " + e.getMessage());
		}
		assertTrue("Check instance validity - 1", under.getState() == ComponentInstance.VALID);
		
		ComponentFactory fact1 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-1");
		ComponentFactory fact2 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-2");
		ComponentFactory fact3 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-3");
		
		
		fact1.stop();
		assertTrue("Check instance validity - 2", under.getState() == ComponentInstance.VALID);
		
		fact2.stop();
		assertTrue("Check instance validity - 3", under.getState() == ComponentInstance.VALID);
		
		fact3.stop();
		assertTrue("Check instance invalidity", under.getState() == ComponentInstance.INVALID);
		
		fact1.start();
		assertTrue("Check instance validity - 4", under.getState() == ComponentInstance.VALID);
		
		under.dispose();
		fact2.start();
		fact3.start();
	}
	
	public void testFactoryManagementLevel2() {
		ServiceContext sc = Utils.getServiceContext(empty);
		Properties props = new Properties();
		props.put("instance.name","under");
		ComponentInstance under = null;
		try {
			under = bar1Factory.createComponentInstance(props, sc);
		} catch(Exception e) {
			fail("Cannot instantiate under : " + e.getMessage());
		}
		assertTrue("Check instance validity - 1", under.getState() == ComponentInstance.VALID);
		
		ComponentFactory fact1 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-1");
		ComponentFactory fact2 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-2");
		ComponentFactory fact3 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-3");
		
		fact1.stop();
		assertTrue("Check instance validity - 2", under.getState() == ComponentInstance.VALID);
		
		fact2.stop();
		assertTrue("Check instance validity - 3", under.getState() == ComponentInstance.VALID);
		
		fact3.stop();
		assertTrue("Check instance invalidity", under.getState() == ComponentInstance.INVALID);
		
		fact1.start();
		assertTrue("Check instance validity - 4", under.getState() == ComponentInstance.VALID);
		
		under.dispose();
		fact2.start();
		fact3.start();
	}
	
	public void testArchitecture() {
		Properties props = new Properties();
		props.put("instance.name","under");
		ComponentInstance under = null;
		try {
			under = bar1Factory.createComponentInstance(props);
		} catch(Exception e) {
			fail("Cannot instantiate under : " + e.getMessage());
		}
		ServiceReference ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), "under");
		assertNotNull("Check architecture availability", ref);
		Architecture arch = (Architecture) getContext().getService(ref);
        CompositeInstanceDescription id = (CompositeInstanceDescription) arch.getInstanceDescription();
		
		assertTrue("Check instance validity - 1", id.getState() == ComponentInstance.VALID);
		InstanceDescription[] contained = id.getContainedInstances();
		assertEquals("Check contained instances count", contained.length, 1);
		assertEquals("Check instance name" , id.getName(), "under");
		assertEquals("Check component type name" , id.getComponentDescription().getName(), "composite.bar.1");
		
		ComponentFactory fact1 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-1");
		ComponentFactory fact2 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-2");
		ComponentFactory fact3 = (ComponentFactory) Utils.getFactoryByName(getContext(), "COMPO-FooBarProviderType-3");
		
		fact1.stop();
		assertTrue("Check instance validity - 2", under.getState() == ComponentInstance.VALID);
		ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), "under");
		assertNotNull("Check architecture availability", ref);
		arch = (Architecture) getContext().getService(ref);
		//id = arch.getInstanceDescription();
		assertTrue("Check instance validity - 1", id.getState() == ComponentInstance.VALID);
		contained = id.getContainedInstances();
		assertEquals("Check contained instances count", contained.length, 1);
		assertEquals("Check instance name" , id.getName(), "under");
		assertEquals("Check component type name" , id.getComponentDescription().getName(), "composite.bar.1");
		
		fact2.stop();
		assertTrue("Check instance validity - 3", under.getState() == ComponentInstance.VALID);
		ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), "under");
		assertNotNull("Check architecture availability", ref);
		arch = (Architecture) getContext().getService(ref);
		//id = arch.getInstanceDescription();
		assertTrue("Check instance validity - 1", id.getState() == ComponentInstance.VALID);
		contained = id.getContainedInstances();
		assertEquals("Check contained instances count", contained.length, 1);
		assertEquals("Check instance name" , id.getName(), "under");
		assertEquals("Check component type name" , id.getComponentDescription().getName(), "composite.bar.1");

		fact3.stop();
		assertTrue("Check instance invalidity", under.getState() == ComponentInstance.INVALID);
		ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), "under");
		assertNotNull("Check architecture availability", ref);
		arch = (Architecture) getContext().getService(ref);
		//id = arch.getInstanceDescription();
		assertTrue("Check instance validity - 1", id.getState() == ComponentInstance.INVALID);
		contained = id.getContainedInstances();
		assertEquals("Check contained instances count", contained.length, 0);
		assertEquals("Check instance name" , id.getName(), "under");
		assertEquals("Check component type name" , id.getComponentDescription().getName(), "composite.bar.1");

		fact1.start();
		assertTrue("Check instance validity - 4", under.getState() == ComponentInstance.VALID);
		ref = Utils.getServiceReferenceByName(getContext(), Architecture.class.getName(), "under");
		assertNotNull("Check architecture availability", ref);
		arch = (Architecture) getContext().getService(ref);
		//id = arch.getInstanceDescription();
		assertTrue("Check instance validity - 1", id.getState() == ComponentInstance.VALID);
		contained = id.getContainedInstances();
		assertEquals("Check contained instances count", contained.length, 1);
		assertEquals("Check instance name" , id.getName(), "under");
		assertEquals("Check component type name" , id.getComponentDescription().getName(), "composite.bar.1");

		under.dispose();
		fact2.start();
		fact3.start();
	}

}
