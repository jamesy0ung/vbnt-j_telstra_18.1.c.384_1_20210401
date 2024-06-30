package org.apache.felix.ipojo.test.scenarios.util;

import java.util.Dictionary;
import java.util.Properties;

import org.apache.felix.ipojo.ComponentInstance;
import org.apache.felix.ipojo.Factory;
import org.apache.felix.ipojo.Handler;
import org.apache.felix.ipojo.HandlerFactory;
import org.apache.felix.ipojo.ServiceContext;
import org.osgi.framework.BundleContext;
import org.osgi.framework.InvalidSyntaxException;
import org.osgi.framework.ServiceReference;
import org.osgi.service.cm.ManagedServiceFactory;

public class Utils {

    public static Factory getFactoryByName(BundleContext bc, String factoryName) {
        ServiceReference[] refs;
        try {
            refs = bc.getServiceReferences(Factory.class.getName(), "(factory.name=" + factoryName + ")");
            if (refs == null) {
                System.err.println("Cannot get the factory " + factoryName);
                return null;
            }
            return ((Factory) bc.getService(refs[0]));
        } catch (InvalidSyntaxException e) {
            System.err.println("Cannot get the factory " + factoryName + " : " + e.getMessage());
            return null;
        }
    }

    public static HandlerFactory getHandlerFactoryByName(BundleContext bc, String factoryName) {
        ServiceReference[] refs;
        try {
            refs = bc.getServiceReferences(Factory.class.getName(), "(" + Handler.HANDLER_NAME_PROPERTY + "=" + factoryName + ")");
            if (refs == null) {
                System.err.println("Cannot get the factory " + factoryName);
                return null;
            }
            return (HandlerFactory) bc.getService(refs[0]);
        } catch (InvalidSyntaxException e) {
            System.err.println("Cannot get the factory " + factoryName + " : " + e.getMessage());
            return null;
        }
    }

    public static ComponentInstance getComponentInstance(BundleContext bc, String factoryName, Dictionary configuration) {
        Factory fact = getFactoryByName(bc, factoryName);

        if (fact == null) {
            System.err.println("Factory " + factoryName + " not found");
            return null;
        }

        // if(fact.isAcceptable(configuration)) {
        try {
            return fact.createComponentInstance(configuration);
        } catch (Exception e) {
            System.err.println("Cannot create the instance from " + factoryName + " : " + e.getMessage());
            e.printStackTrace();
            return null;
        }
        // }
        // else {
        // System.err.println("Configuration not accepted by : " + factoryName);
        // return null;
        // }
    }

    public static ComponentInstance getComponentInstanceByName(BundleContext bc, String factoryName, String name) {
        Factory fact = getFactoryByName(bc, factoryName);

        if (fact == null) {
            System.err.println("Factory " + factoryName + " not found");
            return null;
        }

        try {
            Properties props = new Properties();
            props.put("instance.name",name);
            return fact.createComponentInstance(props);
        } catch (Exception e) {
            System.err.println("Cannot create the instance from " + factoryName + " : " + e.getMessage());
            e.printStackTrace();
            return null;
        }
    }

    public static ServiceReference[] getServiceReferences(BundleContext bc, String itf, String filter) {
        ServiceReference[] refs = null;
        try {
            refs = bc.getServiceReferences(itf, filter);
        } catch (InvalidSyntaxException e) {
            System.err.println("Invalid Filter : " + filter);
        }
        if (refs == null) {
            return new ServiceReference[0];
        } else {
            return refs;
        }
    }

    public static ServiceReference getServiceReference(BundleContext bc, String itf, String filter) {
        ServiceReference[] refs = null;
        try {
            refs = bc.getServiceReferences(itf, filter);
        } catch (InvalidSyntaxException e) {
            System.err.println("Invalid Filter : " + filter);
        }
        if (refs == null) {
            return null;
        } else {
            return refs[0];
        }
    }

    public static ServiceReference getServiceReferenceByName(BundleContext bc, String itf, String name) {
        ServiceReference[] refs = null;
        String filter = null;
        if (itf.equals(Factory.class.getName()) || itf.equals(ManagedServiceFactory.class.getName())) {
            filter = "(" + "factory.name" + "=" + name + ")";
        } else {
            filter = "(" + "instance.name" + "=" + name + ")";
        }
        try {
            refs = bc.getServiceReferences(itf, filter);
        } catch (InvalidSyntaxException e) {
            System.err.println("Invalid Filter : " + filter);
        }
        if (refs == null) {
            return null;
        } else {
            return refs[0];
        }
    }

    public static Object getServiceObject(BundleContext bc, String itf, String filter) {
        ServiceReference ref = getServiceReference(bc, itf, filter);
        if (ref != null) {
            return bc.getService(ref);
        } else {
            return null;
        }
    }

    public static Object[] getServiceObjects(BundleContext bc, String itf, String filter) {
        ServiceReference[] refs = getServiceReferences(bc, itf, filter);
        if (refs != null) {
            Object[] list = new Object[refs.length];
            for (int i = 0; i < refs.length; i++) {
                list[i] = bc.getService(refs[i]);
            }
            return list;
        } else {
            return new Object[0];
        }
    }

    public static Factory getFactoryByName(ServiceContext bc, String factoryName) {
        ServiceReference[] refs;
        try {
            refs = bc.getServiceReferences(Factory.class.getName(), "(factory.name=" + factoryName + ")");
            if (refs == null) { return null; }
            return ((Factory) bc.getService(refs[0]));
        } catch (InvalidSyntaxException e) {
            System.err.println("Cannot get the factory " + factoryName + " : " + e.getMessage());
            return null;
        }
    }

    public static ComponentInstance getComponentInstance(ServiceContext bc, String factoryName, Dictionary configuration) {
        Factory fact = getFactoryByName(bc, factoryName);

        if (fact == null) { return null; }

        if (fact.isAcceptable(configuration)) {
            try {
                return fact.createComponentInstance(configuration);
            } catch (Exception e) {
                System.err.println(e.getMessage());
                e.printStackTrace();
                return null;
            }
        } else {
            System.err.println("Configuration not accepted by : " + factoryName);
            return null;
        }
    }

    public static ServiceReference[] getServiceReferences(ServiceContext bc, String itf, String filter) {
        ServiceReference[] refs = null;
        try {
            refs = bc.getServiceReferences(itf, filter);
        } catch (InvalidSyntaxException e) {
            System.err.println("Invalid Filter : " + filter);
        }
        if (refs == null) {
            return new ServiceReference[0];
        } else {
            return refs;
        }
    }

    public static ServiceReference getServiceReference(ServiceContext bc, String itf, String filter) {
        ServiceReference[] refs = null;
        try {
            refs = bc.getServiceReferences(itf, filter);
        } catch (InvalidSyntaxException e) {
            System.err.println("Invalid Filter : " + filter);
        }
        if (refs == null) {
            return null;
        } else {
            return refs[0];
        }
    }

    public static ServiceReference getServiceReferenceByName(ServiceContext bc, String itf, String name) {
        ServiceReference[] refs = null;
        String filter = null;
        if (itf.equals(Factory.class.getName()) || itf.equals(ManagedServiceFactory.class.getName())) {
            filter = "(" + "factory.name" + "=" + name + ")";
        } else {
            filter = "(" + "instance.name" + "=" + name + ")";
        }
        try {
            refs = bc.getServiceReferences(itf, filter);
        } catch (InvalidSyntaxException e) {
            System.err.println("Invalid Filter : " + filter);
        }
        if (refs == null) {
            return null;
        } else {
            return refs[0];
        }
    }

    public static Object getServiceObject(ServiceContext bc, String itf, String filter) {
        ServiceReference ref = getServiceReference(bc, itf, filter);
        if (ref != null) {
            return bc.getService(ref);
        } else {
            return null;
        }
    }

    public static Object[] getServiceObjects(ServiceContext bc, String itf, String filter) {
        ServiceReference[] refs = getServiceReferences(bc, itf, filter);
        if (refs != null) {
            Object[] list = new Object[refs.length];
            for (int i = 0; i < refs.length; i++) {
                list[i] = bc.getService(refs[i]);
            }
            return list;
        } else {
            return new Object[0];
        }
    }

}
