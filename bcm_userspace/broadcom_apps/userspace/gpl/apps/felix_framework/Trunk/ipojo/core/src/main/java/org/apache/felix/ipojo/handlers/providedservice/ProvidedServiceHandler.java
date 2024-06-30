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
package org.apache.felix.ipojo.handlers.providedservice;

import java.lang.reflect.Field;
import java.util.Dictionary;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.apache.felix.ipojo.ConfigurationException;
import org.apache.felix.ipojo.HandlerFactory;
import org.apache.felix.ipojo.InstanceManager;
import org.apache.felix.ipojo.Pojo;
import org.apache.felix.ipojo.PrimitiveHandler;
import org.apache.felix.ipojo.architecture.ComponentTypeDescription;
import org.apache.felix.ipojo.architecture.HandlerDescription;
import org.apache.felix.ipojo.architecture.PropertyDescription;
import org.apache.felix.ipojo.handlers.dependency.Dependency;
import org.apache.felix.ipojo.handlers.dependency.DependencyHandler;
import org.apache.felix.ipojo.metadata.Attribute;
import org.apache.felix.ipojo.metadata.Element;
import org.apache.felix.ipojo.parser.FieldMetadata;
import org.apache.felix.ipojo.parser.ManifestMetadataParser;
import org.apache.felix.ipojo.parser.ParseException;
import org.apache.felix.ipojo.parser.ParseUtils;
import org.apache.felix.ipojo.parser.PojoMetadata;
import org.apache.felix.ipojo.util.Callback;
import org.apache.felix.ipojo.util.Logger;
import org.apache.felix.ipojo.util.Property;
import org.osgi.framework.Bundle;
import org.osgi.framework.ServiceReference;

/**
 * Composite Provided Service Handler.
 * This handler manage the service providing for a composition.
 * @author <a href="mailto:dev@felix.apache.org">Felix Project Team</a>
 */
public class ProvidedServiceHandler extends PrimitiveHandler {

    /**
     * The list of the provided service.
     */
    private ProvidedService[] m_providedServices = new ProvidedService[0];

    /**
     * The handler description.
     */
    private ProvidedServiceHandlerDescription m_description;

    /**
     * Add a provided service to the list .
     *
     * @param svc : the provided service to add
     */
    private void addProvidedService(ProvidedService svc) {
        // Verify that the provided service is not already in the array.
        for (int i = 0; i < m_providedServices.length; i++) {
            if (m_providedServices[i] == svc) { return; }
        }

        if (m_providedServices.length > 0) {
            ProvidedService[] newPS = new ProvidedService[m_providedServices.length + 1];
            System.arraycopy(m_providedServices, 0, newPS, 0, m_providedServices.length);
            newPS[m_providedServices.length] = svc;
            m_providedServices = newPS;
        } else {
            m_providedServices = new ProvidedService[] { svc };
        }
    }

    /**
     * Get the array of provided service.
     * @return the list of the provided service.
     */
    public ProvidedService[] getProvidedServices() {
        return m_providedServices;
    }

    /**
     * Configure the handler.
     * @param componentMetadata : the component type metadata
     * @param configuration : the instance configuration
     * @throws ConfigurationException : the metadata are not correct.
     * @see org.apache.felix.ipojo.Handler#configure(org.apache.felix.ipojo.InstanceManager, org.apache.felix.ipojo.metadata.Element, java.util.Dictionary)
     */
    public void configure(Element componentMetadata, Dictionary configuration) throws ConfigurationException {
        m_providedServices = new ProvidedService[0];
        // Create the dependency according to the component metadata
        Element[] providedServices = componentMetadata.getElements("Provides");
        for (int i = 0; i < providedServices.length; i++) {
            String[] serviceSpecifications = ParseUtils.parseArrays(providedServices[i].getAttribute("specifications")); // Set by the initialize component factory.

            // Get the factory policy
            int factory = ProvidedService.SINGLETON_STRATEGY;
            Class custom = null;
            String strategy = providedServices[i].getAttribute("strategy");
            if (strategy == null) {
                strategy = providedServices[i].getAttribute("factory");
            }
            if (strategy != null) {
                if ("singleton".equalsIgnoreCase(strategy)) {
                    factory = ProvidedService.SINGLETON_STRATEGY;
                } else if ("service".equalsIgnoreCase(strategy)) {
                    factory = ProvidedService.SERVICE_STRATEGY;
                } else if ("method".equalsIgnoreCase(strategy)) {
                    factory = ProvidedService.STATIC_STRATEGY;
                } else if ("instance".equalsIgnoreCase(strategy)) {
                    factory = ProvidedService.INSTANCE_STRATEGY;
                } else {
                    // Customized policy
                    try {
                        custom = getInstanceManager().getContext().getBundle().loadClass(strategy);
                        if (! CreationStrategy.class.isAssignableFrom(custom)) {
                            throw new ConfigurationException("The custom creation policy class " + custom.getName() + " does not implement " + CreationStrategy.class.getName());
                        }
                    } catch (ClassNotFoundException e) {
                        throw new ConfigurationException("The custom creation policy class " + strategy + " cannot be loaded " + e.getMessage());

                    }

                }
            }


            // Then create the provided service
            ProvidedService svc = new ProvidedService(this, serviceSpecifications, factory, custom, configuration);

            // Post-Registration callback
            String post = providedServices[i].getAttribute("post-registration");
            if (post != null) {
            	Callback cb = new Callback(post, new Class[] {ServiceReference.class}, false, getInstanceManager());
            	svc.setPostRegistrationCallback(cb);
            }

            post = providedServices[i].getAttribute("post-unregistration");
            if (post != null) {
            	// TODO Can we really send the service reference here ?
            	Callback cb = new Callback(post, new Class[] {ServiceReference.class}, false, getInstanceManager());
            	svc.setPostUnregistrationCallback(cb);
            }

            Element[] props = providedServices[i].getElements("Property");
            if (props != null) {
                //Property[] properties = new Property[props.length];
                Property[] properties = new Property[props.length];
                for (int j = 0; j < props.length; j++) {
                    String name = props[j].getAttribute("name");
                    String value = props[j].getAttribute("value");
                    String type = props[j].getAttribute("type");
                    String field = props[j].getAttribute("field");

                    Property prop = new Property(name, field, null, value, type, getInstanceManager(), this);
                    properties[j] = prop;

                    // Check if the instance configuration has a value for this property
                    Object object = configuration.get(prop.getName());
                    if (object != null) {
                        prop.setValue(object);
                    }

                    if (field != null) {
                        getInstanceManager().register(new FieldMetadata(field, type), this);
                        // Cannot register the property as the interception is necessary
                        // to deal with registration update.
                    }
                }

                // Attach to properties to the provided service
                svc.setProperties(properties);
            }

            Element[] controllers = providedServices[i].getElements("Controller");
            if (controllers != null) {
                if (controllers.length > 1) {
                    throw new ConfigurationException("Cannot have several controller per 'provides' element");
                }

                String field = controllers[0].getAttribute("field");
                if (field == null) {
                    throw new ConfigurationException("The field attribute of a controller is mandatory");
                }

                String v = controllers[0].getAttribute("value");
                boolean value = ! (v != null  && v.equalsIgnoreCase("false"));
                svc.setController(field, value);

                getInstanceManager().register(new FieldMetadata(field, "boolean"), this);
            }

            if (checkProvidedService(svc)) {
                addProvidedService(svc);
            } else {
                StringBuffer itfs = new StringBuffer();
                for (int j = 0; j < serviceSpecifications.length; j++) {
                    itfs.append(' ');
                    itfs.append(serviceSpecifications[j]);
                }
                throw new ConfigurationException("The provided service" + itfs + " is not valid");
            }

            // Initialize the description.
            m_description = new ProvidedServiceHandlerDescription(this, m_providedServices);

        }
    }

    /**
     * Collect interfaces implemented by the POJO.
     * @param specs : implemented interfaces.
     * @param parent : parent class.
     * @param bundle : Bundle object.
     * @return the set of implemented interfaces.
     * @throws ClassNotFoundException : occurs when an interface cannot be loaded.
     */
    private Set computeInterfaces(String[] specs, String parent, Bundle bundle) throws ClassNotFoundException {
        Set result = new HashSet();
        // First iterate on found specification in manipulation metadata
        for (int i = 0; i < specs.length; i++) {
            result.add(specs[i]);
            // Iterate on interfaces implemented by the current interface
            Class clazz = bundle.loadClass(specs[i]);
            collectInterfaces(clazz, result, bundle);
        }

        // Look for parent class.
        if (parent != null) {
            Class clazz = bundle.loadClass(parent);
            collectInterfacesFromClass(clazz, result, bundle);
        }

        return result;
    }

    /**
     * Look for inherited interfaces.
     * @param clazz : interface name to explore (class object)
     * @param acc : set (accumulator)
     * @param bundle : bundle
     * @throws ClassNotFoundException : occurs when an interface cannot be loaded.
     */
    private void collectInterfaces(Class clazz, Set acc, Bundle bundle) throws ClassNotFoundException {
        Class[] clazzes = clazz.getInterfaces();
        for (int i = 0; i < clazzes.length; i++) {
            acc.add(clazzes[i].getName());
            collectInterfaces(clazzes[i], acc, bundle);
        }
    }

    /**
     * Collect interfaces for the given class.
     * This method explores super class to.
     * @param clazz : class object.
     * @param acc : set of implemented interface (accumulator)
     * @param bundle : bundle.
     * @throws ClassNotFoundException : occurs if an interface cannot be load.
     */
    private void collectInterfacesFromClass(Class clazz, Set acc, Bundle bundle) throws ClassNotFoundException {
        Class[] clazzes = clazz.getInterfaces();
        for (int i = 0; i < clazzes.length; i++) {
            acc.add(clazzes[i].getName());
            collectInterfaces(clazzes[i], acc, bundle);
        }
        // Iterate on parent classes
        Class sup = clazz.getSuperclass();
        if (sup != null) {
            collectInterfacesFromClass(sup, acc, bundle);
        }
    }

    /**
     * Check the provided service given in argument in the sense that the metadata are consistent.
     * @param svc : the provided service to check.
     * @return true if the provided service is correct
     * @throws ConfigurationException : the checked provided service is not correct.
     */
    private boolean checkProvidedService(ProvidedService svc) throws ConfigurationException {
        for (int i = 0; i < svc.getServiceSpecifications().length; i++) {
            String specName = svc.getServiceSpecifications()[i];

            // Check service level dependencies
            try {
                Class spec = getInstanceManager().getFactory().loadClass(specName);
                Field specField = spec.getField("specification");
                Object specif = specField.get(null);
                if (specif instanceof String) {
                    Element specification = ManifestMetadataParser.parse((String) specif);
                    Element[] deps = specification.getElements("requires");
                    for (int j = 0; deps != null && j < deps.length; j++) {
                        Dependency dep = getAttachedDependency(deps[j]);
                        if (dep != null) {
                            // Fix service-level dependency flag
                            dep.setServiceLevelDependency();
                        }
                        isDependencyCorrect(dep, deps[j]);
                    }
                } else {
                    throw new ConfigurationException("Service Providing: The specification field of the service specification " + svc.getServiceSpecifications()[i] + " needs to be a String");
                }
            } catch (NoSuchFieldException e) {
                return true; // No specification field
            } catch (ClassNotFoundException e) {
                throw new ConfigurationException("Service Providing: The service specification " + svc.getServiceSpecifications()[i] + " cannot be load");
            } catch (IllegalArgumentException e) {
                throw new ConfigurationException("Service Providing: The field 'specification' of the service specification " + svc.getServiceSpecifications()[i] + " is not accessible : " + e.getMessage());
            } catch (IllegalAccessException e) {
                throw new ConfigurationException("Service Providing: The field 'specification' of the service specification " + svc.getServiceSpecifications()[i] + " is not accessible : " + e.getMessage());
            } catch (ParseException e) {
                throw new ConfigurationException("Service Providing: The field 'specification' of the service specification " + svc.getServiceSpecifications()[i] + " does not contain a valid String : " + e.getMessage());
            }
        }

        return true;
    }

    /**
     * Look for the implementation (i.e. component) dependency for the given service-level requirement metadata.
     * @param element : the service-level requirement metadata
     * @return the Dependency object, null if not found or if the DependencyHandler is not plugged to the instance
     */
    private Dependency getAttachedDependency(Element element) {
        DependencyHandler handler = (DependencyHandler) getHandler(HandlerFactory.IPOJO_NAMESPACE + ":requires");
        if (handler == null) { return null; }

        String identity = element.getAttribute("id");
        if (identity != null) {
            // Look for dependency Id
            for (int i = 0; i < handler.getDependencies().length; i++) {
                if (handler.getDependencies()[i].getId().equals(identity)) { return handler.getDependencies()[i]; }
            }
        }
        // If not found or no id, look for a dependency with the same specification
        String requirement = element.getAttribute("specification");
        for (int i = 0; i < handler.getDependencies().length; i++) {
            if ((handler.getDependencies()[i].getSpecification().getName()).equals(requirement)) { return handler.getDependencies()[i]; }
        }

        return null;
    }

    /**
     * Check the correctness of the implementation dependency against the service level dependency.
     * @param dep : dependency to check
     * @param elem : service-level dependency metadata
     * @throws ConfigurationException  : the service level dependency and the implementation dependency does not match.
     */
    private void isDependencyCorrect(Dependency dep, Element elem) throws ConfigurationException {
        String optional = elem.getAttribute("optional");
        boolean opt = optional != null && optional.equalsIgnoreCase("true");

        String aggregate = elem.getAttribute("aggregate");
        boolean agg = aggregate != null && aggregate.equalsIgnoreCase("true");

        if (dep == null && !opt) {
            throw new ConfigurationException("Service Providing: The requirement " + elem.getAttribute("specification") + " is not present in the implementation and is declared as a mandatory service-level requirement");
        }

        if (dep != null && dep.isAggregate() && !agg) {
            throw new ConfigurationException("Service Providing: The requirement " + elem.getAttribute("specification") + " is aggregate in the implementation and is declared as a simple service-level requirement");
        }

        String filter = elem.getAttribute("filter");
        if (dep != null && filter != null) {
            String filter2 = dep.getFilter();
            if (filter2 == null || !filter2.equalsIgnoreCase(filter)) {
                throw new ConfigurationException("Service Providing: The specification requirement " + elem.getAttribute("specification") + " has not the same filter as declared in the service-level requirement");
            }
        }
    }

    /**
     * Stop the provided service handler.
     *
     * @see org.apache.felix.ipojo.Handler#stop()
     */
    public void stop() {
        //Nothing to do.
    }

    /**
     * Start the provided service handler.
     *
     * @see org.apache.felix.ipojo.Handler#start()
     */
    public void start() {
        // Nothing to do.
    }

    /**
     * Setter Callback Method.
     * Check if the modified field is a property to update the value.
     * @param pojo : the pojo object on which the field is accessed
     * @param fieldName : field name
     * @param value : new value
     * @see org.apache.felix.ipojo.Handler#onSet(Object,
     * java.lang.String, java.lang.Object)
     */
    public void onSet(Object pojo, String fieldName, Object value) {
        // Verify that the field name correspond to a dependency
        for (int i = 0; i < m_providedServices.length; i++) {
            ProvidedService svc = m_providedServices[i];
            boolean update = false;
            for (int j = 0; j < svc.getProperties().length; j++) {
                Property prop = svc.getProperties()[j];
                if (fieldName.equals(prop.getField()) && ! prop.getValue().equals(value)) {
                    // it is the associated property
                    prop.setValue(value);
                    update = true;
                }
            }
            if (update) {
                svc.update();
            }
            if (svc.getController() != null  && svc.getController().getField().equals(fieldName)) {
                if (value instanceof Boolean) {
                    svc.getController().setValue((Boolean) value);
                } else {
                    warn("Boolean value expected for the service controler " + fieldName);
                }
            }
        }
        // Else do nothing
    }

    /**
     * Getter Callback Method.
     * Check if the field is a property to push the stored value.
     * @param pojo : the pojo object on which the field is accessed
     * @param fieldName : field name
     * @param value : value pushed by the previous handler
     * @return the stored value or the previous value.
     * @see org.apache.felix.ipojo.Handler#onGet(Object,
     * java.lang.String, java.lang.Object)
     */
    public Object onGet(Object pojo, String fieldName, Object value) {
        for (int i = 0; i < m_providedServices.length; i++) {
            ProvidedService svc = m_providedServices[i];
            for (int j = 0; j < svc.getProperties().length; j++) {
                Property prop = svc.getProperties()[j];
                if (fieldName.equals(prop.getField())) {
                    // Manage the No Value case.
                    return prop.onGet(pojo, fieldName, value);
                }
            }
            if (svc.getController() != null  && svc.getController().getField().equals(fieldName)) {
                return new Boolean(svc.getController().getValue());
            }
        }
        // Else it is not a property
        return value;
    }

    /**
     * Register the services if the new state is VALID. Unregister the services
     * if the new state is UNRESOLVED.
     *
     * @param state : the new instance state.
     * @see org.apache.felix.ipojo.Handler#stateChanged(int)
     */
    public void stateChanged(int state) {
        // If the new state is INVALID => unregister all the services
        if (state == InstanceManager.INVALID) {
            for (int i = 0; i < m_providedServices.length; i++) {
                m_providedServices[i].unregisterService();
            }
            return;
        }

        // If the new state is VALID => register all the services
        if (state == InstanceManager.VALID) {
            for (int i = 0; i < m_providedServices.length; i++) {
                m_providedServices[i].registerService();
            }
            return;
        }
    }

    /**
     * Adds properties to all provided services.
     * @param dict : dictionary of properties to add
     */
    public void addProperties(Dictionary dict) {
        for (int i = 0; i < m_providedServices.length; i++) {
            m_providedServices[i].addProperties(dict);
            m_providedServices[i].update();
        }
    }

    /**
     * Remove properties form all provided services.
     *
     * @param dict : dictionary of properties to delete.
     */
    public void removeProperties(Dictionary dict) {
        for (int i = 0; i < m_providedServices.length; i++) {
            m_providedServices[i].deleteProperties(dict);
            m_providedServices[i].update();
        }
    }

    /**
     * Build the provided service description.
     * @return the handler description.
     * @see org.apache.felix.ipojo.Handler#getDescription()
     */
    public HandlerDescription getDescription() {
        return m_description;
    }

    /**
     * Reconfigure provided service.
     * @param dict : the new instance configuration.
     * @see org.apache.felix.ipojo.Handler#reconfigure(java.util.Dictionary)
     */
    public void reconfigure(Dictionary dict) {
        for (int j = 0; j < getProvidedServices().length; j++) {
            ProvidedService svc = getProvidedServices()[j];
            Property[] props = svc.getProperties();
            boolean update = false;
            for (int k = 0; k < props.length; k++) {
                if (dict.get(props[k].getName()) != null) {
                    update = true;
                    props[k].setValue(dict.get(props[k].getName()));
                }
            }
            if (update) {
                svc.update();
            }
        }
    }

    /**
     * Initialize the component type.
     * @param desc : component type description to populate.
     * @param metadata : component type metadata.
     * @throws ConfigurationException : occurs when the POJO does not implement any interfaces.
     * @see org.apache.felix.ipojo.Handler#initializeComponentFactory(org.apache.felix.ipojo.architecture.ComponentTypeDescription, org.apache.felix.ipojo.metadata.Element)
     */
    public void initializeComponentFactory(ComponentTypeDescription desc, Element metadata) throws ConfigurationException {
        // Change ComponentInfo
        Element[] provides = metadata.getElements("provides");
        PojoMetadata manipulation = getFactory().getPojoMetadata();

        for (int i = 0; i < provides.length; i++) {
            // First : create the serviceSpecification list
            String[] serviceSpecification = manipulation.getInterfaces();
            String parent = manipulation.getSuperClass();
            Set all = null;
            try {
                all = computeInterfaces(serviceSpecification, parent, desc.getBundleContext().getBundle());
            } catch (ClassNotFoundException e) {
                throw new ConfigurationException("An interface cannot be loaded : " + e.getMessage());
            }

            String serviceSpecificationStr = provides[i].getAttribute("specifications");
            if (serviceSpecificationStr == null) {
                serviceSpecificationStr = provides[i].getAttribute("interface");
                if (serviceSpecificationStr != null) {
                    warn("The 'interface' attribute is deprecated, use the 'specifications' attribute instead of 'interface'");
                }
            }

            if (serviceSpecificationStr != null) {
                List itfs = ParseUtils.parseArraysAsList(serviceSpecificationStr);
                for (int j = 0; j < itfs.size(); j++) {
                    if (! all.contains(itfs.get(j))) {
                        if (parent == null || (parent != null && ! parent.equals((String) itfs.get(j)))) {
                            desc.getFactory().getLogger().log(Logger.WARNING, "The specification " + itfs.get(j) + " is not implemented by " + metadata.getAttribute("classname")
                                    + " it might be a superclass or the class itself.");
                        }
                    }
                }
                all = new HashSet(itfs);
            }

            if (all.isEmpty()) {
                throw new ConfigurationException("Service Providing: Cannot instantiate a provided service : no specifications found (no interfaces implemented by the pojo)");
            }

            StringBuffer specs = null;
            Set set = new HashSet(all);
            set.remove(Pojo.class.getName()); // Remove POJO.
            Iterator iterator = set.iterator();
            while (iterator.hasNext()) {
                String spec = (String) iterator.next();
                desc.addProvidedServiceSpecification(spec);
                if (specs == null) {
                    specs = new StringBuffer("{");
                    specs.append(spec);
                } else {
                    specs.append(',');
                    specs.append(spec);
                }
            }

            specs.append('}');
            provides[i].addAttribute(new Attribute("specifications", specs.toString())); // Add interface attribute to avoid checking in the configure method

            Element[] props = provides[i].getElements("property");
            for (int j = 0; props != null && j < props.length; j++) {
                String name = props[j].getAttribute("name");
                String value = props[j].getAttribute("value");
                String type = props[j].getAttribute("type");
                String field = props[j].getAttribute("field");


                // Get property name :
                if (field != null && name == null) {
                    name = field;
                }

                // Check type if not already set
                if (type == null) {
                    if (field == null) {
                        throw new ConfigurationException("The property " + name + " has neither type nor field.");
                    }
                    FieldMetadata fieldMeta = manipulation.getField(field);
                    if (fieldMeta == null) {
                        throw new ConfigurationException("A declared property was not found in the implementation class : " + field);
                    }
                    type = fieldMeta.getFieldType();
                    props[j].addAttribute(new Attribute("type", type));
                }

                // Is the property set to immutable
                boolean immutable = false;
                String imm = props[j].getAttribute("immutable");
                if (imm != null && imm.equalsIgnoreCase("true")) {
                    immutable = true;
                }

                PropertyDescription pd = new PropertyDescription(name, type, value, immutable);
                desc.addProperty(pd);

                String man = props[j].getAttribute("mandatory");
                if (man != null && man.equalsIgnoreCase("true")) {
                    pd.setMandatory();
                }
            }
        }
    }

}
