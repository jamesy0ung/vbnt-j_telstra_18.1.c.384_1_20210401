/*
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package org.apache.felix.karaf.features.management.internal;

import java.net.URI;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Hashtable;
import java.util.List;

import javax.management.MBeanNotificationInfo;
import javax.management.MBeanRegistration;
import javax.management.MBeanServer;
import javax.management.Notification;
import javax.management.NotificationBroadcasterSupport;
import javax.management.ObjectName;
import javax.management.StandardEmitterMBean;
import javax.management.openmbean.TabularData;

import org.apache.felix.karaf.features.Feature;
import org.apache.felix.karaf.features.FeatureEvent;
import org.apache.felix.karaf.features.FeaturesListener;
import org.apache.felix.karaf.features.FeaturesService;
import org.apache.felix.karaf.features.Repository;
import org.apache.felix.karaf.features.RepositoryEvent;
import org.apache.felix.karaf.features.management.FeaturesServiceMBean;
import org.apache.felix.karaf.features.management.codec.JmxFeature;
import org.apache.felix.karaf.features.management.codec.JmxFeatureEvent;
import org.apache.felix.karaf.features.management.codec.JmxRepository;
import org.apache.felix.karaf.features.management.codec.JmxRepositoryEvent;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceRegistration;

/**
 * Implementation of {@link FeaturesServiceMBean}.
 */
public class FeaturesServiceMBeanImpl extends StandardEmitterMBean implements
    MBeanRegistration, FeaturesServiceMBean {

    private ServiceRegistration registration;

    private BundleContext bundleContext;

	private ObjectName objectName;

	private volatile long sequenceNumber = 0;

	private MBeanServer server;

    private FeaturesService featuresService;

    public FeaturesServiceMBeanImpl() {
        super(FeaturesServiceMBean.class, new NotificationBroadcasterSupport(
            getBroadcastInfo()));
    }

    @Override
    public ObjectName preRegister(MBeanServer server, ObjectName name) throws Exception {
        objectName = name;
        this.server = server;
        return name;
    }

    @Override
    public void postRegister(Boolean registrationDone) {
        registration = bundleContext.registerService(FeaturesListener.class.getName(),
            getFeaturesListener(), new Hashtable());
    }

    @Override
    public void preDeregister() throws Exception {
        registration.unregister();
    }

    /**
     * {@inheritDoc}
     */
    public TabularData getFeatures() throws Exception {
        try {
            List<Feature> allFeatures = Arrays.asList(featuresService.listFeatures());
            List<Feature> insFeatures = Arrays.asList(featuresService.listInstalledFeatures());
            ArrayList<JmxFeature> features = new ArrayList<JmxFeature>();
            for (Feature feature : allFeatures) {
                features.add(new JmxFeature(feature, insFeatures.contains(feature)));
            }
            TabularData table = JmxFeature.tableFrom(features);
            return table;
        } catch (Throwable t) {
            t.printStackTrace();
            return null;
        }
    }

    /**
     * {@inheritDoc}
     */
    public TabularData getRepositories() throws Exception {
        try {
            List<Repository> allRepositories = Arrays.asList(featuresService.listRepositories());
            ArrayList<JmxRepository> repositories = new ArrayList<JmxRepository>();
            for (Repository repository : allRepositories) {
                repositories.add(new JmxRepository(repository));
            }
            TabularData table = JmxRepository.tableFrom(repositories);
            return table;
        } catch (Throwable t) {
            t.printStackTrace();
            return null;
        }
    }

    public void addRepository(String uri) throws Exception {
        featuresService.addRepository(new URI(uri));
    }

    public void removeRepositroy(String uri) throws Exception {
        featuresService.removeRepository(new URI(uri));
    }

    public void installFeature(String name) throws Exception {
        featuresService.installFeature(name);
    }

    public void installFeature(String name, String version) throws Exception {
        featuresService.installFeature(name, version);
    }

    public void uninstallFeature(String name) throws Exception {
        featuresService.uninstallFeature(name);
    }

    public void uninstallFeature(String name, String version) throws Exception {
        featuresService.uninstallFeature(name, version);
    }

    public void setBundleContext(BundleContext bundleContext) {
        this.bundleContext = bundleContext;
    }

    public void setFeaturesService(FeaturesService featuresService) {
        this.featuresService = featuresService;
    }

    public FeaturesListener getFeaturesListener() {
        return new FeaturesListener() {
            public void featureEvent(FeatureEvent event) {
                if (!event.isReplay()) {
                    Notification notification = new Notification(FEATURE_EVENT_TYPE, objectName, sequenceNumber++);
                    notification.setUserData(new JmxFeatureEvent(event).asCompositeData());
                    sendNotification(notification);
                }
            }
            public void repositoryEvent(RepositoryEvent event) {
                if (!event.isReplay()) {
                    Notification notification = new Notification(REPOSITORY_EVENT_TYPE, objectName, sequenceNumber++);
                    notification.setUserData(new JmxRepositoryEvent(event).asCompositeData());
                    sendNotification(notification);
                }
            }
        };
    }


    private static MBeanNotificationInfo[] getBroadcastInfo() {
        String type = Notification.class.getCanonicalName();
        MBeanNotificationInfo info1 = new MBeanNotificationInfo(new String[] {FEATURE_EVENT_EVENT_TYPE},
            type, "Some features notification");
        MBeanNotificationInfo info2 = new MBeanNotificationInfo(new String[] {REPOSITORY_EVENT_EVENT_TYPE},
            type, "Some repository notification");
        return new MBeanNotificationInfo[] {info1, info2};
    }

}