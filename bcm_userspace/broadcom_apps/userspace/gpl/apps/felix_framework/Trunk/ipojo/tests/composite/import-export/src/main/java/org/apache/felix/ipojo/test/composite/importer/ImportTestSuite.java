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
package org.apache.felix.ipojo.test.composite.importer;

import junit.framework.Test;

import org.apache.felix.ipojo.junit4osgi.OSGiTestSuite;
import org.osgi.framework.BundleContext;

public class ImportTestSuite {

	public static Test suite(BundleContext bc) {
	    OSGiTestSuite ots = new OSGiTestSuite("Composite Import Test Suite", bc);
	    ots.addTestSuite(SimpleImport.class);
	    ots.addTestSuite(DelayedSimpleImport.class);
	    ots.addTestSuite(OptionalImport.class);
	    ots.addTestSuite(DelayedOptionalImport.class);
	    ots.addTestSuite(MultipleImport.class);
	    ots.addTestSuite(DelayedMultipleImport.class);
	    ots.addTestSuite(OptionalMultipleImport.class);
	    ots.addTestSuite(DelayedOptionalMultipleImport.class);
	    ots.addTestSuite(FilteredImport.class);
	    ots.addTestSuite(DelayedFilteredImport.class);
		return ots;
	}

}
