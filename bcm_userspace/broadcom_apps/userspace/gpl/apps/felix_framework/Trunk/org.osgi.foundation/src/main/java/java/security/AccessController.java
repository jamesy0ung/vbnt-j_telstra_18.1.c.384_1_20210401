/*
 * $Header: /cvshome/build/ee.foundation/src/java/security/AccessController.java,v 1.6 2006/03/14 01:20:27 hargrave Exp $
 *
 * (C) Copyright 2001 Sun Microsystems, Inc.
 * Copyright (c) OSGi Alliance (2001, 2005). All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package java.security;
public final class AccessController {
	public static void checkPermission(java.security.Permission var0) throws java.security.AccessControlException { }
	public static java.security.AccessControlContext getContext() { return null; }
	public static java.lang.Object doPrivileged(java.security.PrivilegedAction var0) { return null; }
	public static java.lang.Object doPrivileged(java.security.PrivilegedAction var0, java.security.AccessControlContext var1) { return null; }
	public static java.lang.Object doPrivileged(java.security.PrivilegedExceptionAction var0) throws java.security.PrivilegedActionException { return null; }
	public static java.lang.Object doPrivileged(java.security.PrivilegedExceptionAction var0, java.security.AccessControlContext var1) throws java.security.PrivilegedActionException { return null; }
	private AccessController() { } /* generated constructor to prevent compiler adding default public constructor */
}

