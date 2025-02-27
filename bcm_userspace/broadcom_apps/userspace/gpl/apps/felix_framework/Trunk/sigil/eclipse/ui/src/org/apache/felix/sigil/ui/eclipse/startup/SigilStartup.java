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

package org.apache.felix.sigil.ui.eclipse.startup;


import java.util.concurrent.atomic.AtomicInteger;

import org.apache.felix.sigil.eclipse.SigilCore;
import org.apache.felix.sigil.eclipse.job.ResolveProjectsJob;
import org.apache.felix.sigil.repository.IRepositoryChangeListener;
import org.apache.felix.sigil.repository.RepositoryChangeEvent;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.ui.IStartup;


public class SigilStartup implements IStartup
{

    public void earlyStartup()
    {
        final AtomicInteger updateCounter = new AtomicInteger();
        
        // Register a repository change listener to re-run the resolver when repository changes
        SigilCore.getGlobalRepositoryManager().addRepositoryChangeListener( new IRepositoryChangeListener()
        {
            public void repositoryChanged( RepositoryChangeEvent event )
            {
                final int update = updateCounter.incrementAndGet();
                
                Job job = new Job("Pending repository update") {
                    @Override
                    protected IStatus run(IProgressMonitor monitor)
                    {
                        if ( update == updateCounter.get() ) {
                            IWorkspace workspace = ResourcesPlugin.getWorkspace();
                            ResolveProjectsJob job = new ResolveProjectsJob( workspace );
                            job.setSystem( true );
                            job.schedule();
                        }
                        return Status.OK_STATUS;
                    }
                    
                };
                job.setSystem(true);
                job.schedule(100);
            }
        } );
    }
}
