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
package org.apache.felix.ipojo.test.scenarios.component;

import java.util.Collection;
import java.util.Dictionary;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;

import org.apache.felix.ipojo.test.scenarios.service.dependency.service.CheckService;
import org.apache.felix.ipojo.test.scenarios.service.dependency.service.FooService;
import org.osgi.framework.ServiceReference;

public class CollectionCheckService implements CheckService {

    Collection fs;

    int simpleB = 0;

    int objectB = 0;

    int refB = 0;

    int bothB = 0;

    int simpleU = 0;

    int objectU = 0;

    int refU = 0;

    int bothU = 0;
    
    int mapB, mapU, dictB, dictU;

    public boolean check() {
        boolean r = fs.size() != 0;
        Iterator it = fs.iterator();
        while(it.hasNext()) {
            r = r & ((FooService) it.next()).foo();
        }
        return r;
    }

    private boolean getBoolean() {
        return check();
    }

    private int getInt() {
        int r = 0;
        Iterator it = fs.iterator();
        while(it.hasNext()) {
            r = r + ((FooService) it.next()).getInt();
        }
        return r;
    }

    private long getLong() {
        long r = 0;
        Iterator it = fs.iterator();
        while(it.hasNext()) {
            r = r + ((FooService) it.next()).getLong();
        }
        return r;
    }

    private double getDouble() {
        double r = 0.0;
        Iterator it = fs.iterator();
        while(it.hasNext()) {
            r = r + ((FooService) it.next()).getLong();
        }
        return r;
    }

    protected Object doNothing(Object o, String s) {
        return null;
    }

    // private Object getObject() {
    // boolean r = true;
    // for(int i = 0; i < fs.length; i++) {
    // r = r && ((Boolean) fs[i].getObject()).booleanValue();
    // }
    // return new Boolean(r);
    // }

    public Properties getProps() {
        Properties props = new Properties();
        props.put("result", new Boolean(check()));
        props.put("voidB", new Integer(simpleB));
        props.put("objectB", new Integer(objectB));
        props.put("refB", new Integer(refB));
        props.put("bothB", new Integer(bothB));
        props.put("voidU", new Integer(simpleU));
        props.put("objectU", new Integer(objectU));
        props.put("refU", new Integer(refU));
        props.put("bothU", new Integer(bothU));
        props.put("mapB", new Integer(mapB));
        props.put("mapU", new Integer(mapU));
        props.put("dictB", new Integer(dictB));
        props.put("dictU", new Integer(dictU));
        
        props.put("boolean", new Boolean(getBoolean()));
        props.put("int", new Integer(getInt()));
        props.put("long", new Long(getLong()));
        props.put("double", new Double(getDouble()));

        return props;
    }

    public void voidBind() {
        simpleB++;
    }

    public void voidUnbind() {
        simpleU++;
    }

    public void objectBind(FooService o) {
        if (o != null && o instanceof FooService) {
            objectB++;
        }
    }

    public void objectUnbind(FooService o) {
        if (o != null && o instanceof FooService) {
            objectU++;
        }
    }

    public void refBind(ServiceReference sr) {
        if (sr != null) {
            refB++;
        }
    }

    public void refUnbind(ServiceReference sr) {
        if (sr != null) {
            refU++;
        }
    }

    public void bothBind(FooService o, ServiceReference sr) {
        if (o != null && o instanceof FooService && sr != null) {
            bothB++;
        }
    }

    public void bothUnbind(FooService o, ServiceReference sr) {
        if (o != null && o instanceof FooService && sr != null) {
            bothU++;
        }
    }
    
    protected void propertiesMapBind(FooService o, Map props) {
        if(props != null && o != null && o instanceof FooService && props.size() > 0) { mapB++; }
    }   
    protected void propertiesMapUnbind(FooService o, Map props) {
         if(props != null && o != null && o instanceof FooService && props.size() > 0) { mapU++; }
    }
    
    protected void propertiesDictionaryBind(FooService o, Dictionary props) {
        if(props != null && o != null && o instanceof FooService && props.size() > 0) { dictB++; }
    }   
    protected void propertiesDictionaryUnbind(FooService o, Dictionary props) {
        if(props != null && o != null && o instanceof FooService && props.size() > 0) { dictU++; }
    }

}
